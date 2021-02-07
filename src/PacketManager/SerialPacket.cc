// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "SerialPacket.h"
#include <stdexcept>
#include <string.h>
#include <cassert>

namespace RealSenseID
{
namespace PacketManager
{
SerialPacket::SerialPacket()
{
    // zero all bytes before filling the relevant parts
    ::memset(this, 0, sizeof(SerialPacket));

    // fill sync bytes
    sync1 = SyncByte::Sync1;
    sync2 = SyncByte::Sync2;

    // fill protocol version
    protocol_ver = ProtocolVer;

    // fill the end of message bytes
    eom[0] = eom[1] = eom[2] = SyncByte::EndOfMessage;
    eol = '\n';
}

//
// FaPacket impl
//
FaPacket::FaPacket(MsgId id, const char* user_id, char status)
{
    SerialPacket::id = id;
    auto& fa_msg = payload.message.fa_msg;
    constexpr size_t buffer_size = sizeof(fa_msg.user_id);
    static_assert(buffer_size == (PacketManager::MaxUserIdSize + 1), "sizeof(fa_msg.user_id) != (MaxUserIdSize + 1)");

    if (user_id != nullptr)
    {
        // store the user_id in a 16 bytes buffer (max 15 ascii chars + zero terminating byte(s))
        ::strncpy(fa_msg.user_id, user_id, buffer_size - 1);
        fa_msg.user_id[buffer_size - 1] = '\0'; // always null terminated
    }

    fa_msg.fa_status = '0' + status;
    assert(IsFaPacket(*this));
}

FaPacket::FaPacket(MsgId id) : FaPacket(id, nullptr, 0)
{
}

// translate to null terminated user id
const char* FaPacket::GetUserId() const
{
    return payload.message.fa_msg.user_id;
}

char FaPacket::GetStatusCode()
{
    return payload.message.fa_msg.fa_status - '0';
}

//
// DataPacket impl
//
DataPacket::DataPacket(MsgId id, char* data, size_t data_size)
{
    SerialPacket::id = id;
    auto target_size = sizeof(payload.message.data_msg.data);
    if (data_size > target_size)
    {
        throw std::runtime_error("DataPacket ctor: given size exceeds max allowed");
    }
    auto* target_ptr = payload.message.data_msg.data;
    if (data != nullptr)
    {
        ::memcpy(target_ptr, data, data_size);
    }
    assert(IsDataPacket(*this));
}

DataPacket::DataPacket(MsgId id) : DataPacket(id, nullptr, 0)
{
}

const DataMessage& DataPacket::Data() const
{
    return SerialPacket::payload.message.data_msg;
}

bool IsFaPacket(const SerialPacket& packet)
{
    return packet.id >= MsgId::MinFa && packet.id <= MsgId::MaxFa;
}

bool IsDataPacket(const SerialPacket& packet)
{
    return !IsFaPacket(packet);
}
} // namespace PacketManager
} // namespace RealSenseID
