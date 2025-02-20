// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/SerialConfig.h"
#include "RealSenseID/Status.h"
#include <stdint.h>

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
     * @param version Pipe separated string, containing version info for the different firmware modules.
     * @return SerialStatus::Success on success.
     */
    Status QueryFirmwareVersion(std::string& version);

    /**
     * Retrieve device serial number.
     *
     * @param serial String containing the device's serial number.
     * @return SerialStatus::Success on success.
     */
    Status QuerySerialNumber(std::string& serial);

    /**
     * Retrieve device otp version.
     *
     * @param version Char containing the device's otp version.
     * @return SerialStatus::Success on success.
     */
    Status QueryOtpVersion(uint8_t& version);

    /**
     * Send ping packet to device.
     * @return SerialStatus::Success if device responded with a valid ping response.
     */
    Status Ping();

    /**
     * Retrieve logs from the device.
     *
     * @param log String that will be filled with the device's log.
     * @return SerialStatus::Success on success.
     * @Note: Maximum log size is 128kB; therefore, this function allocates up to 128KB and can take approximately 12-14
     * seconds to complete.
     */
    Status FetchLog(std::string& log);

    /**
     * Get color gains packet from device and fill the red, blue values
     * @return SerialStatus::Success on success.
     */
    Status GetColorGains(int& red, int& blue);

    /**
     * Send color gains packet to device. Valid range: 0-511
     * @return SerialStatus::Success on success.
     */
    Status SetColorGains(int red, int blue);

private:
    RealSenseID::DeviceControllerImpl* _impl = nullptr;
};
} // namespace RealSenseID
