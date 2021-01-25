// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/SerialConfig.h"
#include "RealSenseID/Status.h"

namespace RealSenseID
{
class DeviceControllerImpl;

/**
 * Device controller. Responsible for managing the device.
 */
class RSID_API DeviceController
{
public:
    DeviceController();
    ~DeviceController();

    DeviceController(const DeviceController&) = delete;
    DeviceController& operator=(const DeviceController&) = delete;

    /**
     * Connect to device using the given serial config.
     * Reconnect if already connected.
     * 
     * @param config Serial config
     * @return connection status
     */
    Status Connect(const SerialConfig& config);

    /**
     * Disconnect from device
     */
    void Disconnect();

    /**
     * Reboot controlled device.
     *
     * @return True if reboot done successfully and false otherwise.
     */
    bool Reboot();

     /**
     * Retrieve firmware version information.
     *
     * @param outVersion Pipe separated string, containing version info for the different firmware modules.
     * @return SerialStatus::Success on success.
     */
    Status QueryFirmwareVersion(std::string& version);

private:
    RealSenseID::DeviceControllerImpl* _impl = nullptr;
};
} // namespace RealSenseID
