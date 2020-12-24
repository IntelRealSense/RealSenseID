// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "SerialPacket.h"
#include <stdexcept>
#include <string.h>
#include <cassert>

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4996) // supress msvc's strncpy/memcpy warnings
#endif

namespace RealSenseID
{
namespace PacketManager
{
SerialPacket::SerialPacket()
{
    // zero all bytes before filling the relevant parts
    ::memset(this, 0, sizeof(SerialPacket));
}

//
// FaPacket impl
//
FaPacket::FaPacket(MsgId id, const char* user_id, char status)
{
    SerialPacket::payload.id = id;
    auto& fa_msg = SerialPacket::payload.message.fa_msg;
    if (user_id != nullptr)
    {
        ::strncpy(fa_msg.user_id, user_id, sizeof(fa_msg.user_id));
    };

    fa_msg.fa_status = '0' + status;
    assert(IsFaPacket(*this));
}

FaPacket::FaPacket(MsgId id) : FaPacket(id, nullptr, 0)
{
}

// translate to null terminated user id
void FaPacket::GetUserId(char* target, size_t target_size) const
{
    constexpr size_t user_id_size = sizeof(FaMessage::user_id);
    assert(target_size > user_id_size);
    if (target_size <= user_id_size)
    {
        throw std::runtime_error("GetUserId(): given buffer too small");
    }
    auto* user_id = this->payload.message.fa_msg.user_id;
    // user_id ends with '*' in current fw, so translate all '*' to '\0'
    // TODO remove this once fw doesnt use '*' as marker
    for (size_t i = 0; i < user_id_size; i++)
    {
        target[i] = user_id[i] == '*' ? '\0' : user_id[i];
    }
    target[user_id_size] = '\0';
}

char FaPacket::GetStatusCode()
{
    return this->payload.message.fa_msg.fa_status - '0';
}

//
// DataPacket impl
//
DataPacket::DataPacket(MsgId id, char* data, size_t data_size)
{
    SerialPacket::payload.id = id;
    auto target_size = sizeof(payload.message.data_msg.data);
    if (data_size > target_size)
    {
        throw std::runtime_error("DataPacket ctor: given size exeeds max allowed");
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
    return packet.payload.id >= MsgId::MinFa && packet.payload.id <= MsgId::MaxFa;
}

bool IsDataPacket(const SerialPacket& packet)
{
    return !IsFaPacket(packet);
}
} // namespace PacketManager
} // namespace RealSenseID

#ifdef _WIN32
#pragma warning(pop)
#endif
