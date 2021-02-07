// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <mutex>

namespace RealSenseID
{
namespace PacketManager
{
class CyclicBuffer
{
public:
    CyclicBuffer();

    size_t Read(char* destination_buffer, size_t bytes_to_read);
    size_t Write(char* source_buffer, size_t bytes_to_write);

private:
    static const size_t _buffer_size = 65536;
    unsigned char _buffer[_buffer_size];

	size_t _read_index;
	size_t _write_index;
	bool _buffer_full;
	std::mutex _mutex;
};
} // namespace PacketManager
} // namespace RealSenseID