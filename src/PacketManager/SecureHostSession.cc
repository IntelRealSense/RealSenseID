// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "SecureHostSession.h"
#include "PacketSender.h"
#include "Logger.h"
#include "Timer.h"
#include <stdexcept>
#include <string>
#include <cassert>

static const char* LOG_TAG = "SecureHostSession";
static const int MAX_SEQ_NUMBER_DELTA = 20;

namespace RealSenseID
{
namespace PacketManager
{
SecureHostSession::SecureHostSession(SignCallback sign_callback, VerifyCallback verify_callback) :
    _sign_callback(sign_callback), _verify_callback(verify_callback)
{
}

Status SecureHostSession::Start(SerialConnection* serial_conn)
{
    std::lock_guard<std::mutex> lock {_mutex};
    LOG_DEBUG(LOG_TAG, "start session");

    _is_open = false;

    if (serial_conn == nullptr)
    {
        throw std::runtime_error("secure_host_session: serial_connection is null");
    }
    _serial = serial_conn;
    _last_sent_seq_number = 0;
    _last_recv_seq_number = 0;

        
    // Generate ecdh keys and get public key with signature
    MbedtlsWrapper::SignCallback sign_clbk = [this](const unsigned char* buffer, const unsigned int buffer_len,
                                                    unsigned char* out_sig) {
        return this->_sign_callback(buffer, buffer_len, out_sig);
    };
    // Switch to binary mode
    auto status = SwitchToBinary();
    if (status != Status::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed switching to binary mode");
        return status;
    }

    // Generate and send our public key to device
    unsigned char* signed_pubkey = _crypto_wrapper.GetSignedEcdhPubkey(sign_clbk);
    if (!signed_pubkey)
    {
        LOG_ERROR(LOG_TAG, "Failed to generate signed ECDH public key");
        return Status::SecurityError;
    }
    auto ecdhSignedPubKeySize = _crypto_wrapper.GetSignedEcdhPubkeySize();
    DataPacket packet {MsgId::ClientKey, reinterpret_cast<char*>(signed_pubkey), ecdhSignedPubKeySize};

    PacketSender sender {_serial};
    status = sender.Send(packet);
    if (status != Status::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to send ecdh data packet");
        return status;
    }

    // Read device's key and generate shared secret
    status = sender.Recv(packet);
    if (status != Status::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to recv device key response");
        return status;
    }
    if (packet.payload.id != MsgId::ServerKey)
    {
        LOG_ERROR(LOG_TAG, "Mutual authentication failed");
        return Status::SecurityError;
    }

    assert(IsDataPacket(packet));

    // Verify the received key
    MbedtlsWrapper::VerifyCallback verify_clbk = [this](const unsigned char* buffer, const unsigned int buffer_len,
                                                        const unsigned char* sig, const unsigned int sig_len) {
        return _verify_callback(buffer, buffer_len, sig, sig_len);
    };

    auto* data_to_verify = reinterpret_cast<const unsigned char*>(packet.Data().data);
    if (!_crypto_wrapper.VerifyEcdhSignedKey(data_to_verify, verify_clbk))
    {
        LOG_ERROR(LOG_TAG, "Verify key callback failed");
        return Status::SecurityError;
    }

    _is_open = true;
    return Status::Ok;
}

bool SecureHostSession::IsOpen()
{
    std::lock_guard<std::mutex> lock {_mutex};
    return _is_open;
}

SecureHostSession::~SecureHostSession()
{
    LOG_DEBUG(LOG_TAG, "close session");
}

// Encrypt and send packet to the serial connection
Status SecureHostSession::SendPacket(SerialPacket& packet)
{
    std::lock_guard<std::mutex> lock {_mutex};
    return SendPacketImpl(packet);
}

// Wait for any packet until timeout.
// Decrypt the packet.
// Fill the given packet with the decrypted received packet packet.
Status SecureHostSession::RecvPacket(SerialPacket& packet)
{
    std::lock_guard<std::mutex> lock {_mutex};
    return RecvPacketImpl(packet);
}

// Receive packet, decrypt and try to convert to FaPacket
Status SecureHostSession::RecvFaPacket(FaPacket& packet)
{
    std::lock_guard<std::mutex> lock {_mutex};
    auto status = RecvPacketImpl(packet);
    if (status != Status::Ok)
    {
        return status;
    }
    return IsFaPacket(packet) ? Status::Ok : Status::RecvUnexpectedPacket;
}

// Receive packet, decrypt and try to convert to DataPacket
Status SecureHostSession::RecvDataPacket(DataPacket& packet)
{
    std::lock_guard<std::mutex> lock {_mutex};
    auto status = RecvPacketImpl(packet);
    if (status != Status::Ok)
    {
        return status;
    }
    return IsDataPacket(packet) ? Status::Ok : Status::RecvUnexpectedPacket;
}

// private members. non thread safe
Status SecureHostSession::SwitchToBinary()
{
    auto status = _serial->SendBytes(Commands::binary, strlen(Commands::binary));
    if (status != Status::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed sending switch-to-binary command");
        return status;
    }

    // wait for @FBinary
    PacketSender sender {_serial};
    SerialPacket packet;
    Timer t {std::chrono::milliseconds {1000}};
    status = sender.WaitSyncBytes(packet, &t);
    if (status != Status::Ok)
    {
        return status;
    }
    // wait for rest of "Binary" without newline
    char bin_result[32];
    return _serial->RecvBytes(bin_result, strlen(Commands::binary) - 3);
}

Status SecureHostSession::SendPacketImpl(SerialPacket& packet)
{
    // increment and set sequence number in the packet
    packet.payload.sequence_number = ++_last_sent_seq_number;

    // encrypt packet except for sync bytes and msg id
    char* packet_ptr = (char*)&packet;
    char* payload_to_encrypt = &packet_ptr[3];
    constexpr const auto encrypt_length = sizeof(packet.payload) - 1;
    unsigned char temp_encrypted_data[encrypt_length];
    ::memset(temp_encrypted_data, 0, encrypt_length);
    auto ok = _crypto_wrapper.Encrypt((unsigned char*)payload_to_encrypt, temp_encrypted_data, encrypt_length);

    if (!ok)
    {
        LOG_ERROR(LOG_TAG, "Failed encrypting packet");
        return Status::SecurityError;
    }

    // copy back encrypted data in the the packet payload
    ::memcpy(payload_to_encrypt, (char*)temp_encrypted_data, encrypt_length);

    assert(_serial != nullptr);

    PacketSender sender {_serial};
    return sender.Send(packet);
}

// new sequence number should advance by max of MAX_SEQ_NUMBER_DELTA from last number
static bool validate_seq_number(uint32_t last_recv_number, uint32_t seq_number)
{
    return (last_recv_number < seq_number && seq_number <= last_recv_number + MAX_SEQ_NUMBER_DELTA);
}

Status SecureHostSession::RecvPacketImpl(SerialPacket& packet)
{
    assert(_serial != nullptr);
    PacketSender sender {_serial};
    auto status = sender.Recv(packet);
    if (status != Status::Ok)
    {
        return status;
    }

    // decrypt payload except for msg id
    char* payload_to_decrypt = ((char*)&(packet.payload)) + 1;
    auto* data_to_decrypt = reinterpret_cast<const unsigned char*>(payload_to_decrypt);
    constexpr const auto decrypt_length = sizeof(packet.payload) - 1;
    unsigned char temp_decrypted_data[decrypt_length];
    ::memset(temp_decrypted_data, 0, sizeof(temp_decrypted_data));
    auto ok = _crypto_wrapper.Decrypt(data_to_decrypt, temp_decrypted_data, decrypt_length);
    if (!ok)
    {
        LOG_ERROR(LOG_TAG, "Failed decrypting packet");
        return Status::SecurityError;
    }

    // copy back encrypted data in the the packet payload
    ::memcpy(payload_to_decrypt, temp_decrypted_data, decrypt_length);

    // validate sequence number
    auto current_seq = packet.payload.sequence_number;
    if (!validate_seq_number(_last_recv_seq_number, current_seq))
    {
        LOG_ERROR(LOG_TAG, "Invalid sequence number. Last: %zu, Current: %zu", _last_recv_seq_number, current_seq);
        return Status::SecurityError;
    }
    _last_recv_seq_number = current_seq;
    return Status::Ok;
}
} // namespace PacketManager
} // namespace RealSenseID
