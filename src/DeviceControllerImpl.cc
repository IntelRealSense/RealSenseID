// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "DeviceControllerImpl.h"
#include "StatusHelper.h"
#include "PacketManager/SerialPacket.h"
#include "PacketManager/PacketSender.h"
#include "PacketManager/Randomizer.h"
#include "Logger.h"
#include <memory>
#include <string.h>

#ifdef _WIN32
#include "PacketManager/WindowsSerial.h"
#elif UNIX
#include "PacketManager/UnixSerial.h"
#elif ANDROID
#include "PacketManager/AndroidSerial.h"
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
        serial_config.ser_type = static_cast<PacketManager::SerialType>(config.serType);
        serial_config.port = config.port;

#ifdef _WIN32
        _serial = std::make_unique<PacketManager::WindowsSerial>(serial_config);
#elif UNIX
        _serial = std::make_unique<PacketManager::UnixSerial>(serial_config);
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

#ifdef ANDROID
Status DeviceControllerImpl::Connect(int fileDescriptor, int readEndpointAddress, int writeEndpointAddress)
{
    try
    {
        // disconnect if already connected
        _serial.reset();

        _serial =
            std::make_unique<PacketManager::AndroidSerial>(fileDescriptor, readEndpointAddress, writeEndpointAddress);
        return Status::Ok;
        LOG_ERROR(LOG_TAG, "Serial connection method not supported for OS");
        return Status::Error;
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
#endif

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

    std::string constructed_version;

    try
    {
        PacketManager::PacketSender sender {_serial.get()};

        for (int i = 0; i < MAX_NUMBER_OF_MODULES; ++i)
        {
            // request version information for module #i
            char packet_data = static_cast<char>(i);
            PacketManager::DataPacket version_packet {PacketManager::MsgId::Versioning, &packet_data, 1};
            auto status = sender.SendBinary(version_packet);
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

Status DeviceControllerImpl::QuerySerialNumber(std::string& serial)
{
    // clear serial number string to avoid returning garbage
    serial.clear();

    try
    {
        PacketManager::PacketSender sender {_serial.get()};

        PacketManager::DataPacket serial_number_packet {PacketManager::MsgId::SerialNumber};
        auto status = sender.SendBinary(serial_number_packet);
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed sending data packet (status %d)", (int)status);
            return ToStatus(status);
        }

        status = sender.Recv(serial_number_packet);
        if (status != PacketManager::SerialStatus::Ok)
        {
            LOG_ERROR(LOG_TAG, "Failed receiving data packet (status %d)", (int)status);
            return ToStatus(status);
        }

        auto data_size = sizeof(serial_number_packet.payload.message.data_msg.data);
        serial_number_packet.payload.message.data_msg.data[data_size - 1] = '\0';
        serial = serial_number_packet.payload.message.data_msg.data;

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
