// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2025 Intel Corporation. All Rights Reserved.

#include "AndroidSerial.h"
#include "Logger.h"
#include "SerialPacket.h"
#include <string>
#include <cassert>
#include <chrono>
#include <thread>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "CyclicBuffer.h"
#include "Timer.h"

namespace RealSenseID
{
namespace PacketManager
{

static const char* LOG_TAG = "AndroidSerial";
constexpr int XIOCTL_RETRIES = 5;

static void ThrowAndroidError(std::string msg)
{
    throw std::runtime_error(msg + ". GetLastError: " + std::to_string(errno));
}

inline int xioctl(int fh, int request, void* arg)
{
    int retries = XIOCTL_RETRIES;
    int retval = -1;

    // clang-format off
        do {
            retval = ioctl(fh, request, arg);
        } while (
                retval
                && retries--
                && (errno == EINTR)
                );
    // clang-format on
    return retval;
}


void AndroidSerial::StartReadFromDeviceWorkingThread()
{
    _reader_thread = std::thread([this]() {
        const size_t read_buffer_size = 4096;
        char temp_read_buffer[read_buffer_size];
        while (!_is_stopping)
        {
            struct usbdevfs_bulktransfer bulk;
            memset(&bulk, 0, sizeof(bulk));
            bulk.ep = _config.readEndpoint;
            bulk.len = read_buffer_size;
            bulk.data = (void*)temp_read_buffer;
            bulk.timeout = 200 + 4 * read_buffer_size;
            int ioctl_result = xioctl(_config.fileDescriptor, USBDEVFS_BULK, &bulk);
            if (-1 < ioctl_result)
            {
                size_t ioctl_positive_result = ::abs(ioctl_result);
                // LOG_DEBUG(LOG_TAG, "ioctl returned %d bytes from the device using FileDescriptor(%d) and
                // ReadEndpointAddress(%d)", ioctlResult, m_FileDescriptor, m_ReadEndpointAddress);
                if (ioctl_positive_result == read_buffer_size)
                {
                    LOG_ERROR(LOG_TAG, "ioctl returned %d. %s", errno, strerror(errno));
                }
                else
                {
                    size_t actual_bytes_writen = _device_read_buffer.Write(temp_read_buffer, ioctl_positive_result);
                    if (actual_bytes_writen != ioctl_positive_result)
                    {
                        LOG_ERROR(LOG_TAG, "Intermediate buffer out of space!");
                        assert(false);
                    }
                }
            }
        }
    });
}

SerialStatus AndroidSerial::SendBytes(const char* buffer, size_t n_bytes)
{
    if (n_bytes == 0)
    {
        LOG_ERROR(LOG_TAG, "[snd] Attempt to send 0 bytes");
        return SerialStatus::SendFailed;
    }

    struct usbdevfs_bulktransfer bulk;
    memset(&bulk, 0, sizeof(bulk));
    bulk.ep = _config.writeEndpoint;
    bulk.len = n_bytes;
    bulk.data = (void*)buffer;
    bulk.timeout = 200 + 4 * n_bytes;
    auto number_of_bytes_sent = xioctl(_config.fileDescriptor, USBDEVFS_BULK, &bulk);

    if (0 > number_of_bytes_sent)
    {
        int error_number = errno;
        LOG_ERROR(LOG_TAG, "[snd] ioctl failed with error: 0x%x", error_number);
        return SerialStatus::SendFailed;
    }

    if (n_bytes != ::abs(number_of_bytes_sent))
    {
        LOG_ERROR(LOG_TAG, "[snd] Error while writing to serial port");
        return SerialStatus::SendFailed;
    }
    DEBUG_SERIAL(LOG_TAG, "[snd]", buffer, n_bytes);
    return SerialStatus::Ok;
}

AndroidSerial::~AndroidSerial()
{
    _is_stopping = true;
    if (_reader_thread.joinable())
    {
        _reader_thread.join();
    }
}

AndroidSerial::AndroidSerial(const SerialConfig& config) : _config(config)
{
    _is_stopping = false;
    StartReadFromDeviceWorkingThread();
}

SerialStatus AndroidSerial::RecvBytes(char* buffer, size_t n_bytes)
{
    if (n_bytes == 0)
    {
        LOG_ERROR(LOG_TAG, "[rcv] Attempt to recv 0 bytes");
        return SerialStatus::RecvFailed;
    }

    // set timeout to depend on number of bytes needed
    Timer timer {std::chrono::milliseconds {200 + 4 * n_bytes}};
    size_t total_bytes_read = 0;
    while (!timer.ReachedTimeout())
    {
        char* buf_ptr = buffer + total_bytes_read;
        auto last_read_result = _device_read_buffer.Read(buf_ptr, n_bytes - total_bytes_read);
        if (last_read_result > 0)
        {
            total_bytes_read += last_read_result;
            if (total_bytes_read >= n_bytes)
            {
                assert(n_bytes == total_bytes_read);
                DEBUG_SERIAL(LOG_TAG, "[rcv]", buffer, total_bytes_read);
                return SerialStatus::Ok;
            }
        }
        else
        {
            // Allow time for writing to the buffer
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    DEBUG_SERIAL(LOG_TAG, "[rcv]", buffer, total_bytes_read);
    DEBUG_SERIAL(LOG_TAG, "[rcv] Timeout recv %zu bytes. Got only %zu bytes", n_bytes, total_bytes_read);
    return SerialStatus::RecvTimeout;
}

} // namespace PacketManager
} // namespace RealSenseID
