// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

//
// Serial packet spec (little endian for all uint16_t and uint32_t fields)
//

#include <cstdint>
#include <string.h>

#pragma pack(push)
#pragma pack(1)

namespace RealSenseID
{
namespace PacketManager
{
struct FaMessage
{
    char user_id[16]; // ascii only. '\0' terminated
    char fa_status;   // ascii status number (e.g. '0', '1', etc.)
};

struct DataMessage
{
    char data[128]; // any binary data
};

enum class SyncByte : char
{
    Sync1 = '@',
    Sync2 = 'F',
    EndOfMessage = '$' // end of message
};

enum class MsgId : char
{
    MinFa = 'A',
    Reply = 'Y',
    Result = 'R',
    Progress = 'P',
    Hint = 'H',
    Enroll = 'E',
    Authenticate = 'A',
    AuthenticateLoop = 'O',
    RemoveUser = 'D',
    RemoveAllUsers = 'C',
    Cancel = 'S',
    MaxFa = 'Z',
    ReplyDevice = 'r',
    InitDevice = 'i',
    SetAuthSettings = 'k',
    Features = 'f',
    ClientKey = 'c',
    ServerKey = 's'
};

struct SerialPacket
{
    SyncByte sync1;
    SyncByte sync2;

    struct
    {
        MsgId id; //'A'-'Z' fa message, 'a-'z' data message
        uint32_t sequence_number;
        char sequence_number_pad[12]; // 0 pad for encryption purposes
        union {
            FaMessage fa_msg;
            DataMessage data_msg;
        } message;
    } payload;

    // end of message bytes
    SyncByte eom[3]; // "$$$"
    char eol;
    SerialPacket();
};

//
// fa packet
//
struct FaPacket : public SerialPacket
{
    FaPacket(MsgId id, const char* user_id, char status);
    FaPacket(MsgId id);
    void GetUserId(char* target, size_t target_size) const;
    char GetStatusCode();
};

// data packet
struct DataPacket : public SerialPacket
{
    // copy data to packet. pad with zeroes if data_size is smaller than actual reserved data size
    DataPacket(MsgId id, char* data, size_t data_size);
    DataPacket(MsgId id);
    const DataMessage& Data() const;
};

bool IsFaPacket(const SerialPacket& packet);   // if MsgId in the 'A'..'Z' range
bool IsDataPacket(const SerialPacket& packet); // if MsgId in the 'a'..'z' range

namespace Commands
{
static const char* binmode0 = "binmode 0\n";
static const char* binmode1 = "binmode 1\n";
static const char* init_debug_uart = "init 0\n";
static const char* init_host_uart = "init 1\n";
static const char* init_usb = "init 2\n";
static const char* binary = "@Fbinary\n";
static const char* reset = "reset\n";
} // namespace Commands
} // namespace PacketManager
}; // namespace RealSenseID

#pragma pack(pop)