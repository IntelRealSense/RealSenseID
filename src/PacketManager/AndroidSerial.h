// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once
#include "SerialConnection.h"
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include "CyclicBuffer.h"

namespace RealSenseID
{
namespace PacketManager
{
class AndroidSerial : public SerialConnection
{
public:
    // explicit AndroidSerial(int file_descriptor, int read_endpoint_address, int write_endpoint_address);
    explicit AndroidSerial(const SerialConfig& config);
    ~AndroidSerial() override;

    // prevent copy or assignment
    // only single connection is allowed to a serial port.
    AndroidSerial(const AndroidSerial&) = delete;
    void operator=(const AndroidSerial&) = delete;

    // send all bytes and return status
    SerialStatus SendBytes(const char* buffer, size_t n_bytes) final;

    // receive all bytes and copy to the buffer
    SerialStatus RecvBytes(char* buffer, size_t n_bytes) final;

private:
    SerialConfig _config;

    void StartReadFromDeviceWorkingThread();
    std::atomic<bool> _is_stopping;

    std::thread _reader_thread;
    CyclicBuffer _device_read_buffer;
};

} // namespace PacketManager
} // namespace RealSenseID
