// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "DeviceControllerImpl.h"
#include "PacketManager/SerialPacket.h"
#include "PacketManager/Timer.h"
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
SerialStatus DeviceControllerImpl::Connect(const SerialConfig& config)
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
        return SerialStatus::Ok;
    }
    catch (const std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return SerialStatus::Error;
    }
    catch (...)
    {
        LOG_ERROR(LOG_TAG, "Unknown exception durting serial connect");
        return SerialStatus::Error;
    }
}

void DeviceControllerImpl::Disconnect()
{
    _serial.reset();
}

bool DeviceControllerImpl::Reboot()
{
    auto status = _serial->SendBytes(PacketManager::Commands::reset, strlen(PacketManager::Commands::reset));
    return (status == PacketManager::Status::Ok);
}
} // namespace RealSenseID
