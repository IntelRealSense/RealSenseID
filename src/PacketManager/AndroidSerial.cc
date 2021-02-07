// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "AndroidSerial.h"
#include "Logger.h"
#include "SerialPacket.h"
#include <string.h>
#include <chrono>
#include <thread>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>

#include <linux/serial.h>
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

static void ThrowAndroidError(std::string msg)
{
    throw std::runtime_error(msg + ". GetLastError: " + std::to_string(errno));
}

void AndroidSerial::StartReadFromDeviceWorkingThread(){
    _worker_thread = std::thread([this]() {
        const size_t read_buffer_size = 4096;
        char temp_read_buffer[read_buffer_size];
        while(false == _stop_read_from_device_working_thread)
        {
            struct usbdevfs_bulktransfer ctrl;
            memset(&ctrl, 0, sizeof(ctrl));
            ctrl.ep = _read_endpoint_address;
            ctrl.len = read_buffer_size;
            ctrl.data = (void*)temp_read_buffer;
            ctrl.timeout = 2000;
            int ioctl_result = ioctl(_file_descriptor, USBDEVFS_BULK, &ctrl);
            if (-1 < ioctl_result) {
                size_t ioctl_positive_result = ::abs(ioctl_result);
                //LOG_DEBUG(LOG_TAG, "ioctl returned %d bytes from the device using FileDescriptor(%d) and ReadEndpointAddress(%d)", ioctlResult, m_FileDescriptor, m_ReadEndpointAddress);
                if(ioctl_positive_result == read_buffer_size){
                    LOG_DEBUG(LOG_TAG, "ioctl returned %d. %s", errno, strerror(errno));
                } else {
                    size_t actual_bytes_writen = _read_from_device_buffer.Write(temp_read_buffer, ioctl_positive_result);
                    if (actual_bytes_writen != ioctl_positive_result) {
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
	struct usbdevfs_bulktransfer ctrl;
    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.ep = _write_endpoint_address;
    ctrl.len = n_bytes;
    ctrl.data = (void*)buffer;
    ctrl.timeout = 1000;
    int number_of_bytes_sent = ioctl(_file_descriptor, USBDEVFS_BULK, &ctrl);

    if(0 > number_of_bytes_sent)
    {
        int error_number = errno;
        LOG_ERROR(LOG_TAG, "ioctl failed with error: 0x%x", error_number);
        return SerialStatus::SendFailed;
    }

    if (n_bytes != ::abs(number_of_bytes_sent))
    {
        LOG_ERROR(LOG_TAG, "Error while writing to serial port");
        return SerialStatus::SendFailed;
    }
    DEBUG_SERIAL(LOG_TAG, "[snd]", buffer, n_bytes);
    return SerialStatus::Ok;
}

AndroidSerial::~AndroidSerial()
{
    _stop_read_from_device_working_thread = true;
    auto ignored_status = this->SendBytes(Commands::binmode0, strlen(Commands::binmode0));
    (void)ignored_status;
    if (_worker_thread.joinable())
    {
        _worker_thread.join();
    }
}

AndroidSerial::AndroidSerial(int file_descriptor, int read_endpoint_address, int write_endpoint_address) :
        _stop_read_from_device_working_thread(false), _file_descriptor(file_descriptor), _read_endpoint_address(read_endpoint_address), _write_endpoint_address(write_endpoint_address)
{
    _stop_read_from_device_working_thread = false;
    StartReadFromDeviceWorkingThread();
    auto* init_cmd = Commands::init_usb;
    auto status = this->SendBytes(init_cmd, strlen(init_cmd));
    if (status != SerialStatus::Ok)
    {
        ThrowAndroidError("Failed to send init command");
    }
}

SerialStatus AndroidSerial::RecvBytes(char* buffer, size_t n_bytes)
{
    if (n_bytes == 0)
    {
        LOG_ERROR(LOG_TAG, "Attempt to recv 0 bytes");
        return SerialStatus::RecvFailed;
    }

    Timer timer {recv_packet_timeout};
    size_t total_bytes_read = 0;
    while (!timer.ReachedTimeout())
    {
        char* buf_ptr = buffer + total_bytes_read;
        auto last_read_result = _read_from_device_buffer.Read(buf_ptr, n_bytes - total_bytes_read);
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
    LOG_DEBUG(LOG_TAG, "Timeout recv %zu bytes. Got only %zu bytes", n_bytes, total_bytes_read);
    return SerialStatus::RecvTimeout;
}

} // namespace PacketManager
} // namespace RealSenseID
