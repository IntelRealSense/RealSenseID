// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "SerialConnection.h"
#include "SerialPacket.h"
#include "CommonTypes.h"
#include "Timer.h"
#include "MbedtlsWrapper.h"
#include <mutex>

// Thread safe secure session manager. sends/receive packets with encryption.
// Session starts on Start(serial_connection*) and ends in destruction.
// Note:
// This class is not responsible for opening/closing the serial connection. It only uses the given one.
namespace RealSenseID
{
namespace PacketManager
{
using SignCallback = std::function<bool(const unsigned char*, const unsigned int, unsigned char*)>;
using VerifyCallback =
    std::function<bool(const unsigned char*, const unsigned int, const unsigned char*, const unsigned int)>;

class SecureHostSession
{
public:
    SecureHostSession(SignCallback signCallback, VerifyCallback verifyCallback);
    ~SecureHostSession();

    SecureHostSession(const SecureHostSession&) = delete;
    SecureHostSession& operator=(const SecureHostSession&) = delete;

    // start the session using the given (already open) serial connection.
    // return Status::Ok on success, or error Status otherwise.
    Status Start(SerialConnection* serial_conn);

    // return true if session is open
    bool IsOpen();

    // send packet
    // return Status::Ok on success, or error status otherwise.
    Status SendPacket(SerialPacket& packet);

    // wait for any packet until timeout.
    // decrypt the packet.
    // fill the given packet with the decrypted received packet packet.
    // return Status::Ok on success, or error status otherwise.
    Status RecvPacket(SerialPacket& packet);

    // wait for fa packet until timeout.
    // decrypt the packet.
    // fill the given packet with the received fa packet.
    // if no fa packet available, return timeout status.
    // if the wrong packet type arrives, return RecvUnexpectedPacket status.
    // return Status::Ok on success, or error status otherwise.
    Status RecvFaPacket(FaPacket& packet);

    // wait for data packet until timeout.
    // decrypt the packet.
    // fill the given packet with the received data packet.
    // if no data packet available, return timeout status.
    // if the wrong packet type arrives, return RecvUnexpectedPacket status.
    // return Status::Ok on success, or error status otherwise.
    Status RecvDataPacket(DataPacket& packet);

private:
    SerialConnection* _serial = nullptr;
    uint32_t _last_sent_seq_number = 0;
    uint32_t _last_recv_seq_number = 0;
    SignCallback _sign_callback;
    VerifyCallback _verify_callback;
    MbedtlsWrapper _crypto_wrapper;
    bool _is_open = false;
    std::mutex _mutex;
    Status SendPacketImpl(SerialPacket& packet);
    Status RecvPacketImpl(SerialPacket& packet);
    Status SwitchToBinary();
};
} // namespace PacketManager
} // namespace RealSenseID
