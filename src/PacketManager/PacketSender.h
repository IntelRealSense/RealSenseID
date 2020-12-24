// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "SerialPacket.h"
#include "CommonTypes.h"

// packet sender for sending/receiving complete serial packets over the serial interface
namespace RealSenseID
{
namespace PacketManager
{
class SerialConnection;
class Timer;
class PacketSender
{
public:
    explicit PacketSender(SerialConnection* serializer);

    // send packet and return Status::ok on success
    Status Send(SerialPacket& packet);

    // receive complete and valid packet (with valid crc)
    // return:
    // Status::Ok on success,
    // Status::RecvTimeout on timeout
    // Status::RecvFailed on other failures
    Status Recv(SerialPacket& target);

    Status WaitSyncBytes(SerialPacket& target, Timer* timeout);

    // return the underlying serial interface
    SerialConnection* SerialIface();

private:
    SerialConnection* _serial;
};
} // namespace PacketManager
} // namespace RealSenseID
