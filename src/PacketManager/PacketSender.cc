// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "PacketSender.h"
#include "SerialConnection.h"
#include "Timer.h"
#include "Logger.h"
#include <thread>
#include <string.h>
#include <cstdint>
#include <stdexcept>

const char* LOG_TAG = "PacketSender";

namespace RealSenseID
{
namespace PacketManager
{
PacketSender::PacketSender(SerialConnection* serial_iface) : _serial {serial_iface}
{
    if (serial_iface == nullptr)
    {
        throw std::runtime_error("PacketSender::PacketSender() - serial_iface* must not be nullptr");
    }
}

SerialStatus PacketSender::Send(SerialPacket& packet)
{
    LOG_DEBUG(LOG_TAG, "Sending packet '%c'", packet.id);

    // send the packet
    auto* packet_ptr = reinterpret_cast<char*>(&packet);
    auto packet_size = sizeof(packet);
    return _serial->SendBytes(packet_ptr, packet_size);
}

SerialStatus PacketSender::SendWithBinary1(SerialPacket& packet)
{
    auto status = _serial->SendBytes(Commands::binary1, ::strlen(Commands::binary1));
    if (status != SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed sending binary1 command");
        return status;
    }
    return this->Send(packet);
}

// end buf points pass the end of current buf
static bool is_eom(const SerialPacket& packet)
{
    return packet.eom[0] == SyncByte::EndOfMessage && packet.eom[1] == SyncByte::EndOfMessage &&
           packet.eom[2] == SyncByte::EndOfMessage && packet.eol == '\n';
}

// keep trying getting the packet until timeout
SerialStatus PacketSender::Recv(SerialPacket& target)
{
    LOG_DEBUG(LOG_TAG, "Waiting packet..");

    Timer timer {recv_packet_timeout};
    // reset the target packet with zeros
    ::memset(reinterpret_cast<char*>(&target), 0, sizeof(target));

    // wait for sync bytes up to timeout
    auto res_status = WaitSyncBytes(target, &timer);
    if (res_status != SerialStatus::Ok)
    {
        return res_status;
    }
    
    // validate protocol version
    res_status = _serial->RecvBytes((char*)&target.protocol_ver, 1);
    if (res_status != SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to recv protocol version byte");
        return res_status;
    }
    if (target.protocol_ver != ProtocolVer)
    {
        LOG_ERROR(LOG_TAG, "Protocol version doesn't match. Expected: %u, Received: %u", ProtocolVer, target.protocol_ver);
        return SerialStatus::VersionMismatch;
    }

    // recv rest of packet (without the sync bytes which we already read)
    auto bytes_to_read = sizeof(target) - 3;
    auto* target_ptr = reinterpret_cast<char*>(&target) + 3;
    res_status = _serial->RecvBytes(target_ptr, bytes_to_read);
    if (res_status != SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to recv rest of packet body (%zu bytes)", bytes_to_read);
        return res_status;
    }

    // make sure we got valid end of message and trailing new line
    if (!is_eom(target))
    {
        LOG_ERROR(LOG_TAG, "Didn't find expected end of message bytes");
        return SerialStatus::RecvFailed;
    }
    LOG_DEBUG(LOG_TAG, "Received packet '%c' after %zu millis", target.id, timer.Elapsed());
    return SerialStatus::Ok;
}

// wait for sync bytes and place them into target
SerialStatus PacketSender::WaitSyncBytes(SerialPacket& target, Timer* timer)
{
    while (!timer->ReachedTimeout())
    {
        auto res_status = _serial->RecvBytes(reinterpret_cast<char*>(&target.sync1), 1);
        if (res_status == SerialStatus::Ok && target.sync1 == SyncByte::Sync1)
        {
            // wait for sync2
            res_status = _serial->RecvBytes(reinterpret_cast<char*>(&target.sync2), 1);
            if (res_status == SerialStatus::Ok && target.sync2 == SyncByte::Sync2)
            {
                return SerialStatus::Ok;
            }
        }
    }
    return SerialStatus::RecvTimeout;
}

SerialConnection* PacketSender::SerialIface()
{
    return _serial;
}


} // namespace PacketManager
} // namespace RealSenseID
