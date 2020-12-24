// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#include "LinuxSerial.h"
#include "CommonTypes.h"
#include "SerialPacket.h"
#include "Timer.h"
#include "Logger.h"
#include <string>
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <errno.h>
#include <cassert>

static const char* LOG_TAG = "LinuxSerial";

static speed_t to_speed_t(unsigned int baudRate)
{
    switch (baudRate)
    {
    case 110:
        return B110;
    case 300:
        return B300;
    case 600:
        return B600;
    case 1200:
        return B1200;
    case 2400:
        return B2400;
    case 4800:
        return B4800;
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    default:
        return -1;
    }
}

namespace RealSenseID
{
namespace PacketManager
{
LinuxSerial::~LinuxSerial()
{
    auto ignored_status = this->SendBytes(Commands::binmode0, strlen(Commands::binmode0));
    (void)ignored_status;
    ::close(_handle);
}

LinuxSerial::LinuxSerial(const SerialConfig& config)
{
    LOG_DEBUG(LOG_TAG, "Opening serial port %s @ %u baudrate", config.port, config.baudrate);
    _handle = ::open(config.port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (_handle < 0)
    {
        throw std::runtime_error("Failed open serial port. errno: " + std::to_string(errno));
    }

    ::fcntl(_handle, F_SETFL, FNDELAY);

    struct termios options;
    tcgetattr(_handle, &options);
    ::memset(&options, 0, sizeof(options));

    auto baudRate = to_speed_t(config.baudrate);
    if (baudRate == -1)
    {
        ::close(_handle);
        throw std::runtime_error("Failed open serial port. Invalid baudrate");
    }

    ::cfsetispeed(&options, baudRate);
    ::cfsetospeed(&options, baudRate);

    // return whatever bytes avaiable after 200ms max
    options.c_cc[VTIME] = 2;
    options.c_cc[VMIN] = 0;
    options.c_cflag |= (CLOCAL | CREAD | CS8);
    options.c_iflag |= (IGNPAR | IGNBRK);

    ::tcsetattr(_handle, TCSANOW, &options);

    // send the "init 1\n"/"init 2\n"
    auto* init_cmd = config.ser_type == SerialType::USB ? Commands::init_usb : Commands::init_host_uart;
    auto status = this->SendBytes(init_cmd, strlen(init_cmd));
    if (status != Status::Ok)
    {
        ::close(_handle);
        throw std::runtime_error("Failed open serial port.");
    }
    ::usleep(100000); // give time to device to enter the required mode

    // send binmode 1\n
    status = this->SendBytes(Commands::binmode1, strlen(Commands::binmode1));
    if (status != Status::Ok)
    {
        ::close(_handle);
        throw std::runtime_error("Failed open serial port.");
    }
    ::usleep(100000); // 100ms
}


Status LinuxSerial::SendBytes(const char* buffer, size_t n_bytes)
{
    DEBUG_SERIAL(LOG_TAG, "[snd]", buffer, n_bytes);

    auto bytes_written = ::write(_handle, buffer, n_bytes);
    auto status = bytes_written == n_bytes ? Status::Ok : Status::SendFailed;
    if (status != Status::Ok)
    {
        LOG_ERROR(LOG_TAG, "Error while writing to serial port");
    }
    return status;
}

// receive all bytes and copy to the buffer or return error status
// timeout after recv_packet_timeout millis
Status LinuxSerial::RecvBytes(char* buffer, size_t n_bytes)
{
    if (n_bytes == 0)
    {
        LOG_ERROR(LOG_TAG, "Attempt to recv 0 bytes");
        return Status::RecvFailed;
    }

    Timer timer {recv_packet_timeout};
    unsigned int total_bytes_read = 0;
    while (!timer.ReachedTimeout())
    {
        unsigned char* buf_ptr = (unsigned char*)buffer + total_bytes_read;
        auto last_read_result = read(_handle, (void*)buf_ptr, n_bytes - total_bytes_read);
        if (last_read_result > 0)
        {
            total_bytes_read += last_read_result;

            if (total_bytes_read >= n_bytes)
            {
                assert(n_bytes == total_bytes_read);
                DEBUG_SERIAL(LOG_TAG, "[rcv]", buffer, n_bytes);
                return Status::Ok;
            }
        }
    }
    DEBUG_SERIAL(LOG_TAG, "[rcv]", buffer, total_bytes_read);
    LOG_DEBUG(LOG_TAG, "Timeout recv %zu bytes. Got only %zu bytes", n_bytes, total_bytes_read);
    return Status::RecvTimeout;
}

} // namespace PacketManager
} // namespace RealSenseID
