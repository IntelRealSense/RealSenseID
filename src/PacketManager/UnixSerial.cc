// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#include "UnixSerial.h"
#include "CommonTypes.h"
#include "SerialPacket.h"
#include "Timer.h"
#include "Logger.h"
#include <string>
#include <stdexcept>
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <errno.h>
#include <cassert>
#include <cmath>

static const char* LOG_TAG = "UnixSerial";

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
UnixSerial::~UnixSerial()
{
    try
    {
        if (_config.ser_type != SerialType::FirmwareUpdate)
        {
            auto ignored_status = this->SendBytes(Commands::binmode0, strlen(Commands::binmode0));
            (void)ignored_status;
        }
        ::close(_handle);
    }
    catch (...)
    {
    }
}

// throw runtime_error if result is negative with a errno message
static void throw_on_error(int result, const char* err_msg, int handle_to_close)
{
    if (result < 0)
    {
        if (handle_to_close != -1)
            ::close(handle_to_close);
        char buf[128];
        ::snprintf(buf, sizeof(buf), "%s. %s (errno %d)", err_msg, strerror(errno), errno);
        throw std::runtime_error(std::string(buf));
    }
}
UnixSerial::UnixSerial(const SerialConfig& config) : _config {config}
{
    LOG_DEBUG(LOG_TAG, "Opening serial port %s type %s baudrate %u", config.port, ToStr(config.ser_type),
              config.baudrate);
    _handle = ::open(config.port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (_handle < 0)
    {
        throw std::runtime_error("Failed open serial port. errno: " + std::to_string(errno));
    }

    //::fcntl(_handle, F_SETFL, FNDELAY);

    struct termios options = {0};

    auto baudRate = to_speed_t(config.baudrate);
    if (baudRate == -1)
    {
        ::close(_handle);
        throw std::runtime_error("Failed open serial port. Invalid baudrate");
    }

    throw_on_error(::cfsetispeed(&options, baudRate), "cfsetispeed", _handle);
    throw_on_error(::cfsetospeed(&options, baudRate), "cfsetospeed", _handle);

    // return whatever bytes avaiable after 200ms max
    options.c_cc[VTIME] = 2;
    options.c_cc[VMIN] = 0;
    options.c_cflag |= (CLOCAL | CREAD | CS8);
    options.c_iflag |= (IGNPAR | IGNBRK);

    throw_on_error(::tcsetattr(_handle, TCSANOW, &options), "tcsetattr", _handle);

    // discard any existing data in input/output buffers
    ::tcflush(_handle, TCIOFLUSH);

    // send "init 1 1\n"/"init 2 1\n"
    if (config.ser_type != SerialType::FirmwareUpdate)
    {
        auto* init_cmd = config.ser_type == SerialType::USB ? Commands::init_usb : Commands::init_host_uart;
        auto status = this->SendBytes(init_cmd, strlen(init_cmd));
        if (status != SerialStatus::Ok)
        {
            ::close(_handle);
            throw std::runtime_error("UnixSerial: Failed open serial port");
        }
    }
}

SerialStatus UnixSerial::SendBytes(const char* buffer, size_t n_bytes)
{    
    size_t bytes_sent = 0;
    while (n_bytes > bytes_sent)
    {
        auto *send_ptr = &buffer[bytes_sent];
        size_t n_bytes_left = n_bytes - bytes_sent;        
        DEBUG_SERIAL(LOG_TAG, "[snd]", send_ptr, n_bytes_left);
        auto write_rv = ::write(_handle, send_ptr, n_bytes_left);
        if (write_rv <= 0)
        {
            LOG_ERROR(LOG_TAG, "Error while sending %zu bytes. errno=%d, sent so far: %zu, write rv=%zu", n_bytes,
                      errno, bytes_sent, write_rv);
            return SerialStatus::SendFailed;
        }
        bytes_sent += static_cast<size_t>(write_rv);
        ::tcdrain(_handle);
#ifdef RSID_DEBUG_SERIAL
        LOG_DEBUG(LOG_TAG, "[snd] Sent %zu/%zu", bytes_sent, n_bytes);        
#endif
    }
    assert(n_bytes == bytes_sent);

    if (n_bytes > 16) 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds {5});
    }

    return SerialStatus::Ok;
}

// receive all bytes and copy to the buffer or return error status
// timeout after recv_packet_timeout millis
SerialStatus UnixSerial::RecvBytes(char* buffer, size_t n_bytes)
{
    if (n_bytes == 0)
    {
        LOG_ERROR(LOG_TAG, "Attempt to recv 0 bytes");
        return SerialStatus::RecvFailed;
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
                return SerialStatus::Ok;
            }
        }
    }
    DEBUG_SERIAL(LOG_TAG, "[rcv]", buffer, total_bytes_read);
    LOG_DEBUG(LOG_TAG, "Timeout recv %zu bytes. Got only %zu bytes", n_bytes, total_bytes_read);
    return SerialStatus::RecvTimeout;
}
} // namespace PacketManager
} // namespace RealSenseID
