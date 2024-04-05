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
    header.sync1 = SyncByte::Sync1;
    header.sync2 = SyncByte::Sync2;

    header.protocol_ver = ProtocolVer;
    header.id = MsgId::None;
    header.payload_size = 0;
}

static int AlignTo32Bytes(int size)
{
    int mod = size % 32;
    if (mod)
        return size + (32 - size % 32);
    return size;
}

//
// FaPacket impl
//
FaPacket::FaPacket(MsgId id, const char* user_id, char status)
{
    header.id = id;
    header.payload_size = static_cast<uint16_t>(AlignTo32Bytes(sizeof(payload.sequence_number) + sizeof(FaMessage)));
    auto& fa_msg = payload.message.fa_msg;
    constexpr size_t buffer_size = sizeof(fa_msg.user_id);
    static_assert(buffer_size == (PacketManager::MaxUserIdSize + 1), "sizeof(fa_msg.user_id) != (MaxUserIdSize + 1)");

    if (user_id != nullptr)
    {
        // store the user_id in a 31 bytes buffer (max 30 ascii chars + zero terminating byte(s))
        ::strncpy(fa_msg.user_id, user_id, buffer_size - 1);
        fa_msg.user_id[buffer_size - 1] = '\0'; // always null terminated
    }

    fa_msg.fa_status = static_cast<char>('0' + status);
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
    return static_cast<char>(payload.message.fa_msg.fa_status - '0');
}

//
// DataPacket impl
//
DataPacket::DataPacket(MsgId id, char* data, size_t data_size)
{
    header.id = id;
    header.payload_size = static_cast<uint16_t>(AlignTo32Bytes(static_cast<int>(sizeof(payload.sequence_number) + data_size)));
    uint32_t target_size = sizeof(payload.sequence_number) + sizeof(payload.message.data_msg.data);
    if (header.payload_size > target_size)
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
    return payload.message.data_msg;
}

bool IsFaPacket(const SerialPacket& packet)
{
    return packet.header.id >= MsgId::MinFa && packet.header.id <= MsgId::MaxFa;
}

bool IsDataPacket(const SerialPacket& packet)
{
    return !IsFaPacket(packet);
}
} // namespace PacketManager
} // namespace RealSenseID
