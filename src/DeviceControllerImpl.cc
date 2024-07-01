// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "DeviceControllerImpl.h"
#include "StatusHelper.h"
#include "PacketManager/SerialPacket.h"
#include "PacketManager/PacketSender.h"
#include "PacketManager/Randomizer.h"
#include "Logger.h"
#include <sstream>
#include <regex>

#ifdef _WIN32
#include "PacketManager/WindowsSerial.h"
#elif LINUX
#include "PacketManager/LinuxSerial.h"
#else
#error "Platform not supported"
#endif //_WIN32

static const char* LOG_TAG = "DeviceControllerImpl";

namespace RealSenseID
{
Status DeviceControllerImpl::Connect(const SerialConfig& config)
{
    try
    {
        // disconnect if already connected
        _serial.reset();
        PacketManager::SerialConfig serial_config;
        serial_config.port = config.port;

#ifdef _WIN32
        _serial = std::make_unique<PacketManager::WindowsSerial>(serial_config);
#elif LINUX
        _serial = std::make_unique<PacketManager::LinuxSerial>(serial_config);
#else
        LOG_ERROR(LOG_TAG, "Serial connection method not supported for OS");
        return Status::Error;
#endif //_WIN32
        return Status::Ok;
    }
    catch (const std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return Status::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception during serial connect");
        return Status::Error;
    }
}

void DeviceControllerImpl::Disconnect()
{
    _serial.reset();
}

bool DeviceControllerImpl::Reboot()
{
    if (!_serial)
    {
        LOG_ERROR(LOG_TAG, "Not connected to a serial port");
        return false;
    }
    auto status = _serial->SendBytes(PacketManager::Commands::reset, strlen(PacketManager::Commands::reset));
    return (status == PacketManager::SerialStatus::Ok);
}

Status DeviceControllerImpl::QueryFirmwareVersion(std::string& version)
{
    // clear output version string to avoid returning garbage
    version.clear();

    try
    {
        std::string version_in_progress;

        {
            auto status = _serial->SendBytes(PacketManager::Commands::version_info,
                                             ::strlen(PacketManager::Commands::version_info));
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed sending version command");
                return ToStatus(status);
            }
        }

        // receive data until no more is available
        constexpr size_t max_buffer_size = 512;
        char buffer[max_buffer_size] = {0};
        for (size_t i = 0; i < max_buffer_size - 1; ++i)
        {
            auto status = _serial->RecvBytes(&buffer[i], 1);

            // timeout is legal for the final byte, because we do not know the expected data size
            if (status == PacketManager::SerialStatus::RecvTimeout)
                break;

            // other error are still not accepted
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed reading version data");
                return ToStatus(status);
            }
        }

        std::stringstream ss(buffer);
        std::string line;
        while (std::getline(ss, line, '\n'))
        {
            static const std::regex module_regex {R"((OPFW|NNLED|DNET|RECOG|YOLO|AS2DLR|NNLAS|NNLEDR) : ([\d\.]+))"};
            std::smatch match;

            auto match_ok = std::regex_search(line, match, module_regex);

            if (match_ok)
            {
                if (!version_in_progress.empty())
                    version_in_progress += '|';

                version_in_progress += match[1].str();
                version_in_progress += ':';
                version_in_progress += match[2].str();
            }
        }

        if (version_in_progress.empty())
        {
            LOG_ERROR(LOG_TAG, "Firmware version received from device is empty");
            return Status::Error;
        }

        version = version_in_progress;

        return Status::Ok;
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return Status::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        return Status::Error;
    }
}

Status DeviceControllerImpl::QuerySerialNumber(std::string& serial)
{
    // clear serial number string to avoid returning garbage
    serial.clear();

    try
    {
        auto send_status = _serial->SendBytes(PacketManager::Commands::device_info,
                                            ::strlen(PacketManager::Commands::device_info));
        if (send_status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending serial number command");
            return ToStatus(send_status);
        }

        // receive data until no more is available
        constexpr size_t max_buffer_size = 128;
        char buffer[max_buffer_size] = {0};
        for (size_t i = 0; i < max_buffer_size - 1; ++i)
        {
            auto status = _serial->RecvBytes(&buffer[i], 1);

            // timeout is legal for the final byte, because we do not know the expected data size
            if (status == PacketManager::SerialStatus::RecvTimeout)
                break;

            // other error are still not accepted
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed reading serial number data");
                return ToStatus(status);
            }
        }

        std::stringstream ss(buffer);
        std::string line;
        while (std::getline(ss, line, '\n'))
        {
            static const std::regex serial_number_regex {R"(SN : \[(.*)\])"};
            std::smatch match;

            auto match_ok = std::regex_search(line, match, serial_number_regex);

            if (match_ok)
            {
                serial = match[1].str();
                break;
            }
        }

        if (serial.empty())
        {
            LOG_ERROR(LOG_TAG, "Serial number received from device is empty");
            return Status::Error;
        }

        return Status::Ok;
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return Status::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        return Status::Error;
    }
}

Status DeviceControllerImpl::QueryOtpVersion(uint8_t& otpVer)
{
    try
    {
        auto send_status = _serial->SendBytes(PacketManager::Commands::otp_ver, ::strlen(PacketManager::Commands::otp_ver));
        if (send_status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending otp version command");
            return ToStatus(send_status);
        }

        // receive data until no more is available
        constexpr size_t max_buffer_size = 128;
        char buffer[max_buffer_size] = {0};
        for (size_t i = 0; i < max_buffer_size - 1; ++i)
        {
            auto status = _serial->RecvBytes(&buffer[i], 1);

            // timeout is legal for the final byte, because we do not know the expected data size
            if (status == PacketManager::SerialStatus::RecvTimeout)
                break;

            // other error are still not accepted
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed reading serial number data");
                return ToStatus(status);
            }
        }

        std::stringstream ss(buffer);
        std::string line;
        std::string otpVerStr;
        while (std::getline(ss, line, '\n'))
        {
            static const std::regex otp_version_regex {R"(otp version is (.*))"};
            std::smatch match;

            auto match_ok = std::regex_search(line, match, otp_version_regex);

            if (match_ok)
            {
                otpVerStr = match[1].str();
                break;
            }
        }

        if (otpVerStr.empty())
        {
            LOG_ERROR(LOG_TAG, "Otp version received from device is empty");
            return Status::Error;
        }

        otpVer = otpVerStr[0];
        return Status::Ok;
    }
    catch (std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return Status::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception");
        return Status::Error;
    }
}

Status DeviceControllerImpl::Ping()
{
    using namespace PacketManager;
    if (!_serial)
    {
        LOG_ERROR(LOG_TAG, "Not connected to a serial port");
        return Status::Error;
    }

    // create a ping data packet with random data
    char random_data[sizeof(DataMessage::data)];
    Randomizer::Instance().GenerateRandom((unsigned char*)random_data, sizeof(random_data));
    DataPacket ping_packet {MsgId::Ping, random_data, sizeof(random_data)};

    PacketSender sender {_serial.get()};
    auto status = sender.SendBinary(ping_packet);
    if (status != SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed sending ping packet (status %d)", (int)status);
        return ToStatus(status);
    }

    SerialPacket response;
    status = sender.Recv(response);
    if (status != SerialStatus::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed receiving data packet (status %d)", (int)status);
        return ToStatus(status);
    }

    if (response.header.id != MsgId::Ping)
    {
        LOG_ERROR(LOG_TAG, "Got unexpected msg id in ping reply: %c", static_cast<char>(response.header.id));
        return Status::Error;
    }

    // check we got same response
    auto is_same = ::memcmp(random_data, response.payload.message.data_msg.data, sizeof(random_data)) == 0;
    if (!is_same)
    {
        LOG_ERROR(LOG_TAG, "got ping reply with different data");
        return Status::Error;
    }
    return Status::Ok;
}
} // namespace RealSenseID
