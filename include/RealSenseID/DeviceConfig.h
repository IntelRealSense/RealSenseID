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
        Rotation_180_Deg = 1,
        Rotation_90_Deg = 2,
        Rotation_270_Deg = 3
    };

    /**
     * @enum SecurityLevel
     * @brief SecurityLevel to allow. (default is Low)
     */
    enum class SecurityLevel
    {
        High = 0,   // high security level
        Medium = 1, // medium security level
        Low = 2,    // low security level
    };

    /**
     * @enum AlgoFlow
     * @brief Algorithms which will be used during authentication
     */
    enum class AlgoFlow
    {
        All = 0,               // spoof and face detection
        FaceDetectionOnly = 1, // face detection only (default)
        SpoofOnly = 2,         // spoof only
        RecognitionOnly = 3    // recognition only
    };


    enum class DumpMode
    {
        None = 0,        // default
        CroppedFace = 1, // sends snapshot of the detected face (as jpg)
        FullFrame = 2,   // sends left+right raw frames with metadata
    };


    /**
     * @brief Defines three confidence levels used by the Matcher during authentication.
     *
     * Each confidence level corresponds to a different set of thresholds, providing the user with the flexibility to
     * choose between three different False Positive Rates (FPR): Low, Medium, and High. Currently, all sets use the
     * thresholds associated with the "Low" confidence level by default.
     */
    enum class MatcherConfidenceLevel
    {
        High = 0,
        Medium = 1,
        Low = 2 // default
    };

    CameraRotation camera_rotation = CameraRotation::Rotation_0_Deg;
    SecurityLevel security_level = SecurityLevel::Low;
    AlgoFlow algo_flow = AlgoFlow::FaceDetectionOnly;
    DumpMode dump_mode = DumpMode::None;
    MatcherConfidenceLevel matcher_confidence_level = MatcherConfidenceLevel::Low;


    /**
     * @brief Specifies the maximum number of consecutive spoofing attempts allowed before the device rejects further
     * authentication requests.
     *
     * Setting this value to 0 disables the check, which is the default behavior. If the number of consecutive spoofing
     * attempts reaches max_spoofs, the device will reject any subsequent authentication requests. To reset this
     * behavior and allow further authentication attempts, the device must be unlocked using the Unlock() API call.
     */
    unsigned char max_spoofs = 0;


    /**
     * @brief Controls whether GPIO toggling is enabled(1) or disabled(0, default) after successful authentication.
     *
     * Set this value to 1 to enable toggling of GPIO pin #1 after each successful authentication.
     * Set this value to 0 to disable GPIO toggling (default).
     *
     * @note Only GPIO pin #1 can be toggled. Other values are not supported.
     */
    int gpio_auth_toggling = 0;
};

RSID_API const char* Description(DeviceConfig::CameraRotation rotation);
RSID_API const char* Description(DeviceConfig::SecurityLevel level);
RSID_API const char* Description(DeviceConfig::AlgoFlow algoMode);
RSID_API const char* Description(DeviceConfig::DumpMode dump_mode);
RSID_API const char* Description(DeviceConfig::MatcherConfidenceLevel matcher_confidence_level);

template <typename OStream>
inline OStream& operator<<(OStream& os, const DeviceConfig::CameraRotation& rotation)
{
    os << Description(rotation);
    return os;
}

template <typename OStream>
inline OStream& operator<<(OStream& os, const DeviceConfig::AlgoFlow& mode)
{
    os << Description(mode);
    return os;
}

template <typename OStream>
inline OStream& operator<<(OStream& os, const DeviceConfig::SecurityLevel& level)
{
    os << Description(level);
    return os;
}


template <typename OStream>
inline OStream& operator<<(OStream& os, const DeviceConfig::DumpMode& dump_mode)
{
    os << Description(dump_mode);
    return os;
}
template <typename OStream>
inline OStream& operator<<(OStream& os, const DeviceConfig::MatcherConfidenceLevel& matcher_confidence_level)
{
    os << Description(matcher_confidence_level);
    return os;
}
} // namespace RealSenseID
