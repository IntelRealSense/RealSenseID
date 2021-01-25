// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "DeviceControllerImpl.h"
#include "StatusHelper.h"
#include "PacketManager/SerialPacket.h"
#include "PacketManager/PacketSender.h"
#include "Logger.h"
#include <memory>

#ifdef _WIN32
#include "PacketManager/WindowsSerial.h"
#elif LINUX
#include "PacketManager/LinuxSerial.h"
#else
#error "Platform not supported"
#endif //  _WIN32

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
        serial_config.ser_type = static_cast<PacketManager::SerialType>(config.serType);
        serial_config.port = config.port;

#ifdef _WIN32
        _serial = std::make_unique<PacketManager::WindowsSerial>(serial_config);
#elif LINUX
        _serial = std::make_unique<PacketManager::LinuxSerial>(serial_config);
#endif
        return Status::Ok;
    }
    catch (const std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return Status::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception durting serial connect");
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

    constexpr unsigned int MAX_NUMBER_OF_MODULES = 20;
    const std::string INVALID_MODULE_INDICATOR = "INVALID";

    PacketManager::PacketSender sender {_serial.get()};

    std::string constructed_version;

    try
    {
        for (int i = 0; i < MAX_NUMBER_OF_MODULES; ++i)
        {                    
            // request version information for module #i
            char packet_data = static_cast<char>(i);
            PacketManager::DataPacket version_packet {PacketManager::MsgId::Versioning, &packet_data, 1};
            auto status = sender.SendWithBinary1(version_packet);
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed sending data packet (status %d)", (int)status);
                return ToStatus(status);
            }

            status = sender.Recv(version_packet);
            if (status != PacketManager::SerialStatus::Ok)
            {
                LOG_ERROR(LOG_TAG, "Failed receiving data packet (status %d)", (int)status);
                return ToStatus(status);
            }
 
            auto data_size = sizeof(version_packet.payload.message.data_msg.data);
            version_packet.payload.message.data_msg.data[data_size - 1] = '\0';
            std::string received_output = version_packet.payload.message.data_msg.data;

            // if the requested module index does not exist, we can stop
            if (received_output.find(INVALID_MODULE_INDICATOR) != std::string::npos)
                break;

            // concat the obtained version info to our output string
            if (i > 0)
                constructed_version += '|';
            constructed_version += received_output;
        }

        // version string was constructed successfully and can be copied to output
        version = constructed_version;

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
} // namespace RealSenseID
