// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "CyclicBuffer.h"
#include <algorithm>
#include "Logger.h"


namespace RealSenseID
{
namespace PacketManager
{
static const char* LOG_TAG = "CyclicBuffer";

CyclicBuffer::CyclicBuffer() : _read_index(0), _write_index(0), _buffer_full(false)
{
}

size_t CyclicBuffer::Read(char* destination_buffer, size_t bytes_to_read)
{
    if (nullptr == destination_buffer)
    {
        LOG_ERROR(LOG_TAG, "The destinationBuffer is NULL");
        return 0;
    }
    if (bytes_to_read == 0)
    {
        return 0;
    }

    size_t actual_bytes_read = 0;

    const std::lock_guard<std::mutex> lock(_mutex);
    if (_write_index > _read_index)
    {
        actual_bytes_read = std::min(_write_index - _read_index, bytes_to_read);
        ::memcpy(destination_buffer, &_buffer[_read_index], actual_bytes_read);

        _read_index += actual_bytes_read;
    }
    else if (_read_index > _write_index || _buffer_full)
    {
        actual_bytes_read = std::min(_buffer_size - _read_index, bytes_to_read);
        ::memcpy(destination_buffer, &_buffer[_read_index], actual_bytes_read);
        _read_index = (_read_index + actual_bytes_read == _buffer_size) ? 0 : _read_index + actual_bytes_read;
        if (bytes_to_read > actual_bytes_read)
        {
            size_t available_bytes_to_read = std::min(_write_index, bytes_to_read - actual_bytes_read);
            ::memcpy(&destination_buffer[actual_bytes_read], _buffer, available_bytes_to_read);

            actual_bytes_read += available_bytes_to_read;
            _read_index += available_bytes_to_read;
        }
    }

    _buffer_full = false;
    return actual_bytes_read;
}

size_t CyclicBuffer::Write(const char* source_buffer, size_t bytes_to_write)
{
    if (nullptr == source_buffer || _buffer_full)
    {
        if (nullptr == source_buffer)
            LOG_ERROR(LOG_TAG, "The destinationBuffer is NULL");
        if (_buffer_full)
            LOG_ERROR(LOG_TAG, "Buffer is full");
        return 0;
    }

    const std::lock_guard<std::mutex> lock(_mutex);
    size_t actual_bytes_written = 0;
    if (_write_index >= _read_index)
    {
        actual_bytes_written = std::min(_buffer_size - _write_index, bytes_to_write);
        ::memcpy(&_buffer[_write_index], source_buffer, actual_bytes_written);
        _write_index = (_write_index + actual_bytes_written == _buffer_size) ? 0 : _write_index + actual_bytes_written;
    }
    if (bytes_to_write > actual_bytes_written)
    {
        size_t available_bytes_to_write = std::min(_read_index - _write_index, bytes_to_write - actual_bytes_written);
        ::memcpy(&_buffer[_write_index], &source_buffer[actual_bytes_written], available_bytes_to_write);

        actual_bytes_written += available_bytes_to_write;
        _write_index += available_bytes_to_write;
    }
    if ((_write_index == _read_index) && (actual_bytes_written > 0))
    {
        _buffer_full = true;
    }

    return actual_bytes_written;
}

} // namespace PacketManager
} // namespace RealSenseID