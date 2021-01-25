// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "SecureHostSession.h"
#include "PacketSender.h"
#include "Logger.h"
#include "Timer.h"
#include "Randomizer.h"
#include <stdexcept>
#include <string.h>
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

SecureHostSession::~SecureHostSession()
{
    LOG_DEBUG(LOG_TAG, "close session");
}

SerialStatus SecureHostSession::Start(SerialConnection* serial_conn)
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
    auto status = SwitchToBinary(_serial, Commands::binary2);
    if (status != SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed switching to binary mode");
        return status;
    }

    // Generate and send our public key to device
    unsigned char* signed_pubkey = _crypto_wrapper.GetSignedEcdhPubkey(sign_clbk);
    if (!signed_pubkey)
    {
        LOG_ERROR(LOG_TAG, "Failed to generate signed ECDH public key");
        return SerialStatus::SecurityError;
    }
    auto signed_pubkey_size = _crypto_wrapper.GetSignedEcdhPubkeySize();
    DataPacket packet {MsgId::HostEcdhKey, (char*)signed_pubkey, signed_pubkey_size};

    PacketSender sender {_serial};
    status = sender.Send(packet);
    if (status != SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to send ecdh data packet");
        return status;
    }

    // Read device's key and generate shared secret
    status = sender.Recv(packet);
    if (status != SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to recv device key response");
        return status;
    }
    if (packet.id != MsgId::DeviceEcdhKey)
    {
        LOG_ERROR(LOG_TAG, "Mutual authentication failed");
        return SerialStatus::SecurityError;
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
        return SerialStatus::SecurityError;
    }

    _is_open = true;
    return SerialStatus::Ok;
}

bool SecureHostSession::IsOpen()
{
    std::lock_guard<std::mutex> lock {_mutex};
    return _is_open;
}

// Encrypt and send packet to the serial connection
SerialStatus SecureHostSession::SendPacket(SerialPacket& packet)
{
    std::lock_guard<std::mutex> lock {_mutex};
    return SendPacketImpl(packet);
}

// Wait for any packet until timeout.
// Decrypt the packet.
// Fill the given packet with the decrypted received packet packet.
SerialStatus SecureHostSession::RecvPacket(SerialPacket& packet)
{
    std::lock_guard<std::mutex> lock {_mutex};
    return RecvPacketImpl(packet);
}

// Receive packet, decrypt and try to convert to FaPacket
SerialStatus SecureHostSession::RecvFaPacket(FaPacket& packet)
{
    std::lock_guard<std::mutex> lock {_mutex};
    auto status = RecvPacketImpl(packet);
    if (status != SerialStatus::Ok)
    {
        return status;
    }
    return IsFaPacket(packet) ? SerialStatus::Ok : SerialStatus::RecvUnexpectedPacket;
}

// Receive packet, decrypt and try to convert to DataPacket
SerialStatus SecureHostSession::RecvDataPacket(DataPacket& packet)
{
    std::lock_guard<std::mutex> lock {_mutex};
    auto status = RecvPacketImpl(packet);
    if (status != SerialStatus::Ok)
    {
        return status;
    }
    return IsDataPacket(packet) ? SerialStatus::Ok : SerialStatus::RecvUnexpectedPacket;
}

// non thread safe
SerialStatus SecureHostSession::SwitchToBinary(SerialConnection* serial_conn, const char* bincommand)
{
    auto status = serial_conn->SendBytes(bincommand, ::strlen(bincommand));
    if (status != SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed sending switch-to-binary command");
    }
    return status;
}

SerialStatus SecureHostSession::SendPacketImpl(SerialPacket& packet)
{   
    // increment and set sequence number in the packet
    packet.payload.sequence_number = ++_last_sent_seq_number;

    // encrypt packet except for sync bytes and msg id
    char* packet_ptr = (char*)&packet;
    char* payload_to_encrypt = ((char*)&(packet.payload));
    constexpr const auto encrypt_length = sizeof(packet.payload);
    unsigned char temp_encrypted_data[encrypt_length];
    ::memset(temp_encrypted_data, 0, encrypt_length);

    // randomize iv for encryption/decryption
    Randomizer::Instance().GenerateRandom(packet.iv, sizeof(packet.iv));
    auto ok = _crypto_wrapper.Encrypt(packet.iv, (unsigned char*)payload_to_encrypt, temp_encrypted_data, encrypt_length);
    if (!ok)
    {
        LOG_ERROR(LOG_TAG, "Failed encrypting packet");
        return SerialStatus::SecurityError;
    }

    // copy back encrypted data in the the packet payload
    ::memcpy(payload_to_encrypt, (char*)temp_encrypted_data, encrypt_length);

    constexpr const auto hmac_length = sizeof(packet.hmac);
    constexpr const auto content_size = sizeof(packet) - hmac_length;
    ok = _crypto_wrapper.CalcHmac((unsigned char*)packet_ptr, content_size, (unsigned char*)packet.hmac);
    if (!ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to calc hmac");
        return SerialStatus::SecurityError;
    }

    assert(_serial != nullptr);

    PacketSender sender {_serial};
    return sender.Send(packet);
}

// new sequence number should advance by max of MAX_SEQ_NUMBER_DELTA from last number
static bool ValidateSeqNumber(uint32_t last_recv_number, uint32_t seq_number)
{
    return (last_recv_number < seq_number && seq_number <= last_recv_number + MAX_SEQ_NUMBER_DELTA);
}

SerialStatus SecureHostSession::RecvPacketImpl(SerialPacket& packet)
{
    assert(_serial != nullptr);
    PacketSender sender {_serial};
    auto status = sender.Recv(packet);
    if (status != SerialStatus::Ok)
    {
        return status;
    }
    
    char* packet_ptr = (char*)&packet;

    // verify hmac of the received packet
    constexpr const auto hmac_length = sizeof(packet.hmac);
    constexpr const auto content_size = sizeof(packet) - hmac_length;
    char hmac[HMAC_256_SIZE_BYTES];
    auto ok = _crypto_wrapper.CalcHmac((unsigned char*)packet_ptr, content_size, (unsigned char*)hmac);
    if (!ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to calc HMAC");
        return SerialStatus::SecurityError;
    }

    static_assert(sizeof(packet.hmac) == sizeof(hmac), "HMAC size mismatch");
    if (::memcmp(packet.hmac, hmac, HMAC_256_SIZE_BYTES))
    {
        LOG_ERROR(LOG_TAG, "HMAC not the same. Packet not valid");
        return SerialStatus::SecurityError;
    }

    // decrypt payload   
    char* payload_to_decrypt = ((char*)&(packet.payload));
    constexpr const auto decrypt_length = sizeof(packet.payload);
    unsigned char temp_decrypted_data[decrypt_length];
    ::memset(temp_decrypted_data, 0, sizeof(temp_decrypted_data));

    ok = _crypto_wrapper.Decrypt(packet.iv, (unsigned char*)payload_to_decrypt, temp_decrypted_data, decrypt_length);
    if (!ok)
    {
        LOG_ERROR(LOG_TAG, "Failed decrypting packet");
        return SerialStatus::SecurityError;
    }

    // copy back encrypted data in the the packet payload
    ::memcpy(payload_to_decrypt, temp_decrypted_data, decrypt_length);

    // validate sequence number
    auto current_seq = packet.payload.sequence_number;
    if (!ValidateSeqNumber(_last_recv_seq_number, current_seq))
    {
        LOG_ERROR(LOG_TAG, "Invalid sequence number. Last: %zu, Current: %zu", _last_recv_seq_number, current_seq);
        return SerialStatus::SecurityError;
    }
    _last_recv_seq_number = current_seq;
    return SerialStatus::Ok;
}
} // namespace PacketManager
} // namespace RealSenseID
