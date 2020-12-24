// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "SerialConnection.h"
#include "SerialPacket.h"
#include "CommonTypes.h"
#include "Timer.h"
#include <mutex>


// Thread safe, non secure session manager. sends/receive packets without any encryption or signing
// session starts on Start() and ends in destruction.
namespace RealSenseID
{
namespace PacketManager
{
class NonSecureHostSession
{
public:
    NonSecureHostSession() = default;
    ~NonSecureHostSession();

    NonSecureHostSession(const NonSecureHostSession&) = delete;
    NonSecureHostSession& operator=(const NonSecureHostSession&) = delete;


    // start the session using the given (already open) serial connection.
    // return Status::ok on success, or error status otherwise.
    Status Start(SerialConnection* serial_conn);

    bool IsOpen();

    // send packet
    Status SendPacket(SerialPacket& packet);

    // wait for any packet until timeout.
    // fill the given packet with the received packet  packet.
    Status RecvPacket(SerialPacket& packet);

    // wait for fa packet until timeout.
    // fill the given packet with the received fa packet.
    // if no fa packet available, return timeout status.
    // if the wrong packet type arrives, return RecvUnexpectedPacket status.
    Status RecvFaPacket(FaPacket& packet);

    // wait for data packet until timeout.
    // fill the given packet with the received data packet.
    // if no data packet available, return timeout status.
    // if the wrong packet type arrives, return RecvUnexpectedPacket status.
    Status RecvDataPacket(DataPacket& packet);

private:
    SerialConnection* _serial = nullptr;
    std::mutex _mutex;
};
} // namespace PacketManager
} // namespace RealSenseID
