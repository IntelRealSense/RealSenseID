// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#pragma once
#include "SerialConnection.h"

namespace RealSenseID
{
namespace PacketManager
{
class LinuxSerial : public SerialConnection
{
public:
    explicit LinuxSerial(const SerialConfig& config);
    ~LinuxSerial() override;

    // prevent copy or assignment
    // only single connection is allowed to a serial port.
    LinuxSerial(const LinuxSerial&) = delete;
    LinuxSerial operator=(const LinuxSerial&) = delete;

    // send all bytes and return status
    SerialStatus SendBytes(const char* buffer, size_t n_bytes) final;

    // receive all bytes and copy to the buffer
    SerialStatus RecvBytes(char* buffer, size_t n_bytes) final;

private:
    int _handle = -1;
};
} // namespace PacketManager
} // namespace RealSenseID
