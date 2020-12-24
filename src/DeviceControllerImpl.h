// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/SerialConfig.h"
#include "RealSenseID/SerialStatus.h"
#include "PacketManager/SerialConnection.h"
#include <memory>

namespace RealSenseID
{
class DeviceControllerImpl
{
public:
    DeviceControllerImpl() = default;
    ~DeviceControllerImpl() = default;

    // connect to device using the given serial config
    // reconnect if already connected.
    SerialStatus Connect(const SerialConfig& config);

    // disconnect if connected
    void Disconnect();

    DeviceControllerImpl(const DeviceControllerImpl&) = delete;
    DeviceControllerImpl& operator=(const DeviceControllerImpl&) = delete;

    bool Reboot();

private:
    std::unique_ptr<PacketManager::SerialConnection> _serial;
};
} // namespace RealSenseID
