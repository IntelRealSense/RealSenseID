// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseIDExports.h"

namespace RealSenseID
{
struct RSID_API DeviceConfig
{
    /**
     * @enum CameraRotation
     * @brief Camera rotation.
     */
    enum class CameraRotation
    {
        Rotation_0_Deg = 0, // default
        Rotation_180_Deg = 1
    };
    CameraRotation camera_rotation = CameraRotation::Rotation_0_Deg;

    /**
     * @enum SecurityLevel
     * @brief SecurityLevel to allow
     */
    enum class SecurityLevel
    {
        High = 0,  // high security, no mask support, all AS algo(s) will be activated
        Medium = 1, // default mode to support masks, only main AS algo will be activated.  
        RecognitionOnly = 2 // configures device to run recognition only without AS
    };
    SecurityLevel security_level = SecurityLevel::Medium;

    /**
     * @enum PreviewMode
     * @brief Defines preview mode
     * All modes,except VGA, supported only in advanced mode feature, please check if FW supports it using QueryAdvancedMode API.
     */
    enum class PreviewMode
    {
        VGA = 0,       // default
        FHD_Rect = 1,  // result frame with face rect. Supported only in advance mode
        Dump = 2       // dump all frames. Supported only in advance mode
    };
    PreviewMode preview_mode = PreviewMode::VGA;

    bool advanced_mode = false;
};

RSID_API const char* Description(DeviceConfig::CameraRotation rotation);
RSID_API const char* Description(DeviceConfig::SecurityLevel level);
RSID_API const char* Description(DeviceConfig::PreviewMode previewMode);

template <typename OStream>
inline OStream& operator<<(OStream& os, const DeviceConfig::CameraRotation& rotation)
{
    os << Description(rotation);
    return os;
}

template <typename OStream>
inline OStream& operator<<(OStream& os, const DeviceConfig::SecurityLevel& level)
{
    os << Description(level);
    return os;
}

template <typename OStream>
inline OStream& operator<<(OStream& os, const DeviceConfig::PreviewMode& previewMode)
{
    os << Description(previewMode);
    return os;
}
} // namespace RealSenseID
