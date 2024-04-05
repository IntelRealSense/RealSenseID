// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <chrono>

// enable [[nodiscard]] if c++17 is available
#if __cplusplus >= 201703L
#define RSID_NO_DISCARD [[nodiscard]]
#else
#define RSID_NO_DISCARD
#endif //__cplusplus

namespace RealSenseID
{
namespace PacketManager
{

struct SerialConfig
{
    const char* port = nullptr;
    unsigned int baudrate = 115200;
    unsigned char bytesize = 8;
    unsigned char stopbits = 0; // 0=1 stopbits, 1=1.5 stopbits, 2=2 stopbits
    unsigned char parity = 0;
};

enum class RSID_NO_DISCARD SerialStatus
{
    Ok = 100,
    OpenFailed,
    SendFailed,
    RecvTimeout,
    RecvFailed,
    RecvUnexpectedPacket,
    SecurityError,
    VersionMismatch,
    CrcError,
    LicenseError,
    LicenseCheck
};

using timeout_t = std::chrono::milliseconds;
} // namespace PacketManager
} // namespace RealSenseID