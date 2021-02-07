// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once
#include "SerialConnection.h"
#include "CyclicBuffer.h"
#include <memory>
#include <functional>
#include <thread>
#include <pthread.h>
#include <atomic>

namespace RealSenseID
{
namespace PacketManager
{
class AndroidSerial : public SerialConnection
{
public:
	explicit AndroidSerial(int file_descriptor, int read_endpoint_address, int write_endpoint_address);
    ~AndroidSerial();

	// prevent copy or assignment
    // only single connection is allowed to a serial port.
    AndroidSerial(const AndroidSerial&) = delete;
    void operator=(const AndroidSerial&) = delete;

	// send all bytes and return status
    SerialStatus SendBytes(const char* buffer, size_t n_bytes) final;

    // receive all bytes and copy to the buffer
	SerialStatus RecvBytes(char* buffer, size_t n_bytes) final;

private:

	int _file_descriptor;
	int _read_endpoint_address;
	int _write_endpoint_address;

    void StartReadFromDeviceWorkingThread();
    std::atomic<bool> _stop_read_from_device_working_thread;

    std::thread _worker_thread;
    CyclicBuffer _read_from_device_buffer;
};

} // namespace PacketManager
} // namespace RealSenseID