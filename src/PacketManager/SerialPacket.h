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
        static const unsigned char ProtocolVer = 2;
        static const size_t MaxUserIdSize = 30;

        struct FaMessage
        {
            char user_id[MaxUserIdSize + 1]; // ascii only. '\0' terminated
            char fa_status;                  // ascii status number (e.g. '0', '1', etc.)
        };

        struct DataMessage
        {
            char data[1948]; // any binary data to complete packet total size to exactly 2008B)
        };

        enum class SyncByte : char
        {
            Sync1 = '@',
            Sync2 = 'F'
        };

enum class MsgId : char
{
    None = '-',
    MinFa = 'A',
    Authenticate = 'A',
    DetectSpoof = 'B',
    RemoveAllUsers = 'C',
    RemoveUser = 'D',
    Enroll = 'E',
    Hint = 'H',    
    SecureFaceprintsEnroll = 'N',
    SecureFaceprintsAuthenticate = 'Q',
    Progress = 'P',
    Result = 'R',
    EnrollFaceprintsExtraction = 'T',
    AuthenticateFaceprintsExtraction = 'X',
    Reply = 'Y',
    MaxFa = 'Z',
    HostEcdsaKey = 'a',
    DeviceEcdsaKey = 'b',
    HostEcdhKey = 'c',
    DeviceEcdhKey = 'd',    
    Faceprints = 'f',
    FaceDetected = 'g',
    GetNumberOfUsers = 'n',
    StartSession = 'o',
    Ping = 'p',
    QueryDeviceConfig = 'q',
    SetDeviceConfig = 's',
    StandBy = 't',
    GetUserIds = 'u',    
    SecureFaceprintsBeginSecureSession = 'i',
    SecureFaceprintsEndSecureSession = 'j',
    SecureFaceprintsOnSecureSessionReady = 'k',
    SecureFaceprintsOnSecureSessionCmd = 'l',
    SecureFaceprintsOnSecureSessionCmdResp = 'm',
    SecureFaceprintsFaceprintsReady = 'r',
    SetUserFeatures = 'x',
    GetUserFeatures = 'y',

};

        struct SerialPacket
        {
            struct
            {
                SyncByte sync1;
                SyncByte sync2;

                unsigned char protocol_ver;
                MsgId id; //'A'-'Z' fa message, 'a-'z' data message
                unsigned char iv[16];
                uint16_t payload_size;
            } header;
            struct
            {
                uint32_t sequence_number;
                union {
                    FaMessage fa_msg;
                    DataMessage data_msg;
                } message;
            } payload;
            char hmac[32]; // if security is enabled it will store hmac calculation
            uint16_t crc;
            SerialPacket();
        };

        //
        // fa packet
        //
        struct FaPacket : public SerialPacket
        {
            FaPacket(MsgId id, const char* user_id, char status);
            FaPacket(MsgId id);
            const char* GetUserId() const;
            char GetStatusCode();
        };

        // data packet
        struct DataPacket : public SerialPacket
        {
            // copy data to packet. pad with zeros if data_size is smaller than actual reserved data size
            DataPacket(MsgId id, char* data, size_t data_size);
            DataPacket(MsgId id);
            const DataMessage& Data() const;
        };

        bool IsFaPacket(const SerialPacket& packet);   // if MsgId in the 'A'..'Z' range
        bool IsDataPacket(const SerialPacket& packet); // if MsgId in the 'a'..'z' range

        namespace Commands
        {
            static const char* face_api = "\r\n__FACE_API__\r\n";
            static const char* face_cancel = "\r\n__FACE_CANCEL__\r\n";
            static const char* version_info = "\r\nbspver\r\n";
            static const char* device_info = "\r\nbspver -device\r\n";
            static const char* reset = "\r\nreset\r\n";
        } // namespace Commands
    } // namespace PacketManager
}; // namespace RealSenseID

#pragma pack(pop)