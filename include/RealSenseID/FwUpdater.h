// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/RealSenseIDExports.h"
#include "RealSenseID/Status.h"

#include <string>
#include <vector>

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

    // TODO: Document
    struct UpdatePolicyInfo
    {
        enum class UpdatePolicy
        {
            CONTINOUS,
            OPFW_FIRST,
            REQUIRE_INTERMEDIATE_FW,
            NOT_ALLOWED
        };
        UpdatePolicy policy;
        std::string intermediate;
    };


    /**
     * Extracts the firmware and recognition version from the firmware package, as well as all the modules names.
     *
     * @param[in] binPath Path to the firmware binary file.
     * @param[out] outFwVersion Operational firmware (OPFW) version string.
     * @param[out] outRecognitionVersion Recognition model version string.
     * @param[out] moduleNames Names of modules found in the binary file.
     * @return True if extraction succeeded and false otherwise.
     */
    bool ExtractFwInformation(const char* binPath, std::string& outFwVersion, std::string& outRecognitionVersion,
                              std::vector<std::string>& moduleNames) const;

    /**
     * Check SKU version used in the binary file and answer whether the device supports it.
     *
     * @param[in] settings Firmware update settings.
     * @param[in] binPath Path to the firmware binary file.
     * @param[out] expectedSkuVer SKU version of the firmware binary file.
     * @param[out] deviceSkuVer SKU version of the device.
     * @return True if expectedSkuVer == deviceSkuVer and false otherwise.
     */
    bool IsSkuCompatible(const Settings& settings, const char* binPath, int& expectedSkuVer, int& deviceSkuVer) const;

    /**
     * Performs a firmware update for the modules listed in moduleNames
     *
     * @param[in] handler Responsible for handling events triggered during the update.
     * @param[in] settings Firmware update settings.
     * @param[in] binPath Path to the firmware binary file.
     * @param[in] moduleNames list of module names to update.
     * @return OK if update succeeded matching error status if it failed.
     */
    Status UpdateModules(EventHandler* handler, Settings settings, const char* binPath, const std::vector<std::string>& moduleNames) const;

    // TODO: document
    UpdatePolicyInfo DecideUpdatePolicy(const Settings& settings, const char* binPath) const;
};
} // namespace RealSenseID
