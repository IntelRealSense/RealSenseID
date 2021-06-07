// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/SerialConfig.h"
#include "RealSenseID/Status.h"
#include "PacketManager/SerialConnection.h"
#ifdef ANDROID
#include "RealSenseID/AndroidSerialConfig.h"
#endif
#include <memory>

namespace RealSenseID
{
class DeviceControllerImpl
{
public:
    DeviceControllerImpl() = default;
    ~DeviceControllerImpl() = default;

    DeviceControllerImpl(const DeviceControllerImpl&) = delete;
    DeviceControllerImpl& operator=(const DeviceControllerImpl&) = delete;

    Status Connect(const SerialConfig& config);
#ifdef ANDROID
    Status Connect(const AndroidSerialConfig& config);
#endif
    void Disconnect();

    bool Reboot();
    Status QueryFirmwareVersion(std::string& version);
    Status QuerySerialNumber(std::string& serial);
    Status Ping();

private:
    std::unique_ptr<PacketManager::SerialConnection> _serial;
};
} // namespace RealSenseID
