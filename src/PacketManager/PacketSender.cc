// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "PacketSender.h"
#include "SerialConnection.h"
#include "Timer.h"
#include "Logger.h"
#include <thread>
#include <string.h>
#include <cstdint>

const char* LOG_TAG = "PacketSender";

namespace RealSenseID
{
namespace PacketManager
{
PacketSender::PacketSender(SerialConnection* serial_iface) : _serial {serial_iface}
{
}

Status PacketSender::Send(SerialPacket& packet)
{
    LOG_DEBUG(LOG_TAG, "Sending packet '%c'", packet.payload.id);

    // fill sync bytes
    packet.sync1 = SyncByte::Sync1;
    packet.sync2 = SyncByte::Sync2;

    // fill the end of message bytes
    packet.eom[0] = packet.eom[1] = packet.eom[2] = SyncByte::EndOfMessage;
    packet.eol = '\n';

    // send the packet
    auto* packet_ptr = reinterpret_cast<char*>(&packet);
    auto packet_size = sizeof(packet);
    return _serial->SendBytes(packet_ptr, packet_size);
}

// end buf points pass the end of current buf
static bool is_eom(const SerialPacket& packet)
{
    return packet.eom[0] == SyncByte::EndOfMessage && packet.eom[1] == SyncByte::EndOfMessage &&
           packet.eom[2] == SyncByte::EndOfMessage && packet.eol == '\n';
}

// keep trying getting the packet until timeout
Status PacketSender::Recv(SerialPacket& target)
{
    LOG_DEBUG(LOG_TAG, "Waiting packet..");

    Timer timer {recv_packet_timeout};
    // reset the target packet with zeros
    ::memset(reinterpret_cast<char*>(&target), 0, sizeof(target));

    // wait for sync bytes up to timeout
    auto res_status = WaitSyncBytes(target, &timer);
    if (res_status != Status::Ok)
    {
        return res_status;
    }

    // recv rest of packet (without the sync bytes which we already read)
    auto bytes_to_read = sizeof(target) - 2;
    auto* target_ptr = reinterpret_cast<char*>(&target.payload);
    res_status = _serial->RecvBytes(target_ptr, bytes_to_read);
    if (res_status != Status::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to recv rest of packet body (%zu bytes)", bytes_to_read);
        return res_status;
    }

    // make sure we got valid end of message and trailing new line
    if (!is_eom(target))
    {
        LOG_ERROR(LOG_TAG, "Didn't find expected end of message bytes");
        return Status::RecvFailed;
    }
    LOG_DEBUG(LOG_TAG, "Received packet '%c'", target.payload.id);
    return Status::Ok;
}

// wait for sync bytes and place them into target
Status PacketSender::WaitSyncBytes(SerialPacket& target, Timer* timer)
{
    while (!timer->ReachedTimeout())
    {
        auto res_status = _serial->RecvBytes(reinterpret_cast<char*>(&target.sync1), 1);
        if (res_status == Status::Ok && target.sync1 == SyncByte::Sync1)
        {
            // wait for sync2
            res_status = _serial->RecvBytes(reinterpret_cast<char*>(&target.sync2), 1);
            if (res_status == Status::Ok && target.sync2 == SyncByte::Sync2)
            {
                return Status::Ok;
            }
        }
        // try to avoid busy wait on recv failures
        if (res_status != Status::Ok && res_status != Status::RecvTimeout)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds {20});
        }
    }
    return Status::RecvTimeout;
}

SerialConnection* PacketSender::SerialIface()
{
    return _serial;
}
} // namespace PacketManager
} // namespace RealSenseID
