// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/RealSenseIDExports.h"
#include "RealSenseID/Status.h"
#include "RealSenseID/AndroidSerialConfig.h"

#include <string>

namespace RealSenseID
{
/**
 * FwUpdater class.
 * Handles firmware update operations.
 */
class RSID_API FwUpdater
{
public:
    /**
     * Firmware update related settings.
     */
    struct Settings
    {
        const char* port = nullptr; // serial port to perform the update on
        bool force_full = false;    // if true update all modules and blocks regardless of crc checks
#ifdef ANDROID
        AndroidSerialConfig android_config;
#endif
    };

    /**
     * User defined callback for firmware update events.
     * Callback will be used to provide feedback to the client.
     */
    struct EventHandler
    {
        virtual ~EventHandler() = default;

        /**
         * Called to inform the client of the overall firmware update progress.
         *
         * @param[in] progress Current firmware update progress, range: 0.0f - 1.0f.
         */
        virtual void OnProgress(float progress) = 0;
    };

    FwUpdater() = default;
    ~FwUpdater() = default;

    /**
     * Extracts the firmware version from the firmware package.
     *
     * @param[in] binPath Path to the firmware binary file.
     * @param[out] outFwVersion Output version string.
     * @return True if extraction succeeded and false otherwise.
     */
    bool ExtractFwVersion(const char* binPath, std::string& outFwVersion) const;

    /**
     * Performs a firmware update.
     *
     * @param[in] handler Responsible for handling events triggered during the update.
     * @param[in] Settings Firmware update settings.
     * @param[in] binPath Path to the firmware binary file.
     * @return True if extraction succeeded and false otherwise.
     */
    Status Update(EventHandler* handler, Settings settings, const char* binPath) const;
};
} // namespace RealSenseID
