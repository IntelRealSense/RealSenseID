// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "WindowsSerial.h"
#include "SerialPacket.h"
#include "CommonTypes.h"
#include "Logger.h"

#include <string>
#include <stdexcept>

static const char* LOG_TAG = "WindowsSerial";

static void ThrowWinError(std::string msg)
{
    throw std::runtime_error(msg + ". GetLastError: " + std::to_string(::GetLastError()));
}

namespace RealSenseID
{
namespace PacketManager
{
WindowsSerial::WindowsSerial(const SerialConfig& config) : _config {config}
{
    DCB dcbSerialParams = {0};
    std::string port = std::string("\\\\.\\") + _config.port;
    LOG_DEBUG(LOG_TAG, "Opening serial port %s type %s", config.port,
              config.ser_type == SerialType::USB ? "usb" : "uart");
    _handle = ::CreateFileA(port.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (_handle == INVALID_HANDLE_VALUE)
    {
        ThrowWinError("Failed to open serial port");
    }

    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(_handle, &dcbSerialParams))
    {
        // LOG_ERROR(LOG_TAG, "Error getting state");
        ::CloseHandle(_handle);
        ThrowWinError("Failed to open serial port");
    }
    dcbSerialParams.BaudRate = _config.baudrate;
    dcbSerialParams.ByteSize = _config.bytesize;
    dcbSerialParams.Parity = _config.parity;
    dcbSerialParams.StopBits = _config.stopbits;

    if (!SetCommState(_handle, &dcbSerialParams))
    {
        ::CloseHandle(_handle);
        ThrowWinError("Failed to open serial port");
    }

    COMMTIMEOUTS timeouts = {0};

    // make sure recv_packet_timeout is in millis and use it for timeout
    static_assert(std::is_same<decltype(recv_packet_timeout), std::chrono::milliseconds>::value,
                  "recv_packet_timeout must be millis");
    timeouts.ReadTotalTimeoutConstant = static_cast<DWORD>(recv_packet_timeout.count());
    timeouts.WriteTotalTimeoutConstant = 1000;
    if (!SetCommTimeouts(_handle, &timeouts))
    {
        ::CloseHandle(_handle);
        ThrowWinError("Failed to open serial port");
    }

    // send the "init 1\n"/"init 2\n"
    auto* init_cmd = _config.ser_type == SerialType::USB ? Commands::init_usb : Commands::init_host_uart;
    auto status = this->SendBytes(init_cmd, strlen(init_cmd));
    if (status != Status::Ok)
    {
        ::CloseHandle(_handle);
        ThrowWinError("Failed to open serial port");
    }
    ::Sleep(100); // give time to device to enter the required mode

    // send binmode 1\n
    status = this->SendBytes(Commands::binmode1, strlen(Commands::binmode1));
    if (status != Status::Ok)
    {
        ::CloseHandle(_handle);
        ThrowWinError("Failed to open serial port");
    }

    ::Sleep(100); // give time to device to enter the required mode
}

WindowsSerial::~WindowsSerial()
{
    if (_handle != INVALID_HANDLE_VALUE)
    {
        auto ignored_status = this->SendBytes(Commands::binmode0, strlen(Commands::binmode0));
        (void)ignored_status;
        ::CloseHandle(_handle);
    }
}

Status WindowsSerial::SendBytes(const char* buffer, size_t n_bytes)
{
    DWORD bytes_to_write = static_cast<DWORD>(n_bytes);
    DWORD bytes_written = 0;

    DEBUG_SERIAL(LOG_TAG, "[snd]", buffer, n_bytes);

    if (!::WriteFile(_handle, buffer, bytes_to_write, &bytes_written, NULL) || bytes_written != bytes_to_write)
    {
        LOG_ERROR(LOG_TAG, "Error while writing to serial port");
        return Status::SendFailed;
    }
    else
    {
        return Status::Ok;
    }
}

Status WindowsSerial::RecvBytes(char* buffer, size_t n_bytes)
{
    DWORD bytes_to_read = static_cast<DWORD>(n_bytes);
    DWORD bytes_actual_read = 0;

    if (!::ReadFile(_handle, (LPVOID)buffer, bytes_to_read, &bytes_actual_read, NULL))
    {
        LOG_ERROR(LOG_TAG, "Error while reading from serial port");
        return Status::RecvFailed;
    }

    if (bytes_actual_read != bytes_to_read)
    {
        if (bytes_to_read != 1)
        {
            // log only if not waiting for sync bytes, where it is expected to timeout sometimes
            LOG_DEBUG(LOG_TAG, "Timeout reading %d bytes. Got only %d", bytes_to_read, bytes_actual_read);
        }
        return Status::RecvTimeout;
    }
    else
    {
        DEBUG_SERIAL(LOG_TAG, "[rcv]", buffer, bytes_actual_read);
        return Status::Ok;
    }
}
} // namespace PacketManager
} // namespace RealSenseID
