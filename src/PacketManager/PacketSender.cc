// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "PacketSender.h"
#include "SerialConnection.h"
#include "Timer.h"
#include "Logger.h"
#include "Crc16.h"
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
    LOG_DEBUG(LOG_TAG, "Sending packet '%c'", packet.header.id);

    // send the headers + payload
    auto* packet_ptr = reinterpret_cast<const char*>(&packet);
    auto packet_size = sizeof(packet.header) + packet.header.payload_size;
    auto status = _serial->SendBytes(packet_ptr, packet_size);
    if (status != SerialStatus::Ok)
    {
        return status;
    }

    // send the hmac
    status = _serial->SendBytes(packet.hmac, sizeof(packet.hmac));
    if (status != SerialStatus::Ok)
    {
        return status;
    }

    // send crc
    auto crc = CalcCrc(packet);    
    status = _serial->SendBytes(reinterpret_cast<const char*>(&crc), sizeof(crc));
    return status;
}

SerialStatus PacketSender::SendBinary(SerialPacket& packet)
{
    auto status = _serial->SendBytes(Commands::binary, ::strlen(Commands::binary));
    if (status != SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed sending binary command");
        return status;
    }
    return Send(packet);
}

// keep trying getting the packet until timeout
SerialStatus PacketSender::Recv(SerialPacket& target)
{
    LOG_DEBUG(LOG_TAG, "Waiting packet..");

    Timer timer {recv_packet_timeout};
    // reset the target packet with zeros
    ::memset(reinterpret_cast<char*>(&target), 0, sizeof(target));

    // wait for sync bytes up to timeout
    auto status = WaitSyncBytes(target, &timer);
    if (status != SerialStatus::Ok)
    {
        return status;
    }

    // validate protocol version
    status = _serial->RecvBytes((char*)&target.header.protocol_ver, 1);
    if (status != SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to recv protocol version byte");
        return status;
    }
    if (target.header.protocol_ver != ProtocolVer)
    {
        LOG_ERROR(LOG_TAG, "Protocol version doesn't match. Expected: %u, Received: %u", ProtocolVer,
                  target.header.protocol_ver);
        return SerialStatus::VersionMismatch;
    }

    // recv rest of packet header (without the sync bytes and protocol version which we already read)
    auto bytes_to_read = sizeof(target.header) - 3;
    auto* target_ptr = reinterpret_cast<char*>(&target) + 3;
    status = _serial->RecvBytes(target_ptr, bytes_to_read);
    if (status != SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to recv rest of packet header (%zu bytes)", bytes_to_read);
        return status;
    }

    if (target.header.payload_size > sizeof(SerialPacket::payload))
    {
        LOG_ERROR(LOG_TAG, "Packet size is bigger than payload max size");
        return SerialStatus::RecvFailed;
    }

    // recv packet payload
    target_ptr = reinterpret_cast<char*>(&target.payload);
    status = _serial->RecvBytes(target_ptr, target.header.payload_size);
    if (status != SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to recv packet payload (%zu bytes)", target.header.payload_size);
        return status;
    }

    // recv packet hmac
    status = _serial->RecvBytes(target.hmac, sizeof(target.hmac));
    if (status != SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to recv packet hmac (%zu bytes)", sizeof(target.hmac));
        return status;
    }

    // recv packet crc
    status = _serial->RecvBytes(reinterpret_cast<char*>(&target.crc), sizeof(target.crc));
    if (status != SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed to recv packet crc (%zu bytes)", sizeof(target.crc));
        return status;
    }

    // validate crc
    auto expected_crc = CalcCrc(target);
    if (expected_crc != target.crc)
    {
        LOG_ERROR(LOG_TAG, "Got invalid crc. Expected: %u. Actual: %u", expected_crc, target.crc);
        return SerialStatus::CrcError;
    }

    LOG_DEBUG(LOG_TAG, "Received packet '%c' after %zu millis", target.header.id, timer.Elapsed());
    return SerialStatus::Ok;
}

// wait for sync bytes and place them into target
SerialStatus PacketSender::WaitSyncBytes(SerialPacket& target, Timer* timer)
{
    while (!timer->ReachedTimeout())
    {
        auto status = _serial->RecvBytes(reinterpret_cast<char*>(&target.header.sync1), 1);
        if (status == SerialStatus::Ok && target.header.sync1 == SyncByte::Sync1)
        {
            // wait for sync2
            status = _serial->RecvBytes(reinterpret_cast<char*>(&target.header.sync2), 1);
            if (status == SerialStatus::Ok && target.header.sync2 == SyncByte::Sync2)
            {
                return SerialStatus::Ok;
            }
        }
    }
    return SerialStatus::RecvTimeout;
}

uint16_t PacketSender::CalcCrc(const SerialPacket& packet)
{    
    auto* packet_ptr = reinterpret_cast<const char*>(&packet);
    auto crc = Crc16(packet_ptr, sizeof(packet) - sizeof(packet.crc));
    static_assert(sizeof(packet.crc) == sizeof(crc), "packet.crc and crc size mismatch");
    return crc;
}
} // namespace PacketManager
} // namespace RealSenseID
