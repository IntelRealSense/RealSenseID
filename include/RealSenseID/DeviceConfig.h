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
        High = 0,   // high security, no mask support, all AS algo(s) will be activated
        Medium = 1, // default mode to support masks, only main AS algo will be activated.
    };
    SecurityLevel security_level = SecurityLevel::Medium;

    /**
     * @enum AlgoFlow
     * @brief Algorithms which will be used during authentication
     */
    enum class AlgoFlow
    {
        All = 0,               // default
        FaceDetectionOnly = 1, // face detection only
        SpoofOnly = 2,         // spoof only
        RecognitionOnly = 3    // recognition only
    };
    AlgoFlow algo_flow = AlgoFlow::All;

    /**
     * @enum FaceSelectionPolicy
     * @brief To run authentication on all (up to 5) detected faces vs single (closest) face
     */
    enum class FaceSelectionPolicy
    {
        Single = 0, // default, run authentication on closest face
        All = 1     // run authentication on all (up to 5) detected faces
    };
    FaceSelectionPolicy face_selection_policy = FaceSelectionPolicy::Single;

    /**
     * @enum PreviewMode
     * @brief Defines preview mode
     * Dump supported only in advanced mode feature, please check if FW supports it using QueryAdvancedMode API.
     */
    enum class PreviewMode
    {
        MJPEG_1080P = 0, // 1080p mjpeg
        MJPEG_720P = 1,  // 720p mjpeg
        RAW10_1080P = 2  // 1080p raw10
    };

    PreviewMode preview_mode = PreviewMode::MJPEG_1080P;

    bool advanced_mode = false;
};

RSID_API const char* Description(DeviceConfig::CameraRotation rotation);
RSID_API const char* Description(DeviceConfig::SecurityLevel level);
RSID_API const char* Description(DeviceConfig::PreviewMode previewMode);
RSID_API const char* Description(DeviceConfig::AlgoFlow algoMode);
RSID_API const char* Description(DeviceConfig::FaceSelectionPolicy policy);

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
inline OStream& operator<<(OStream& os, const DeviceConfig::PreviewMode& previewMode)
{
    os << Description(previewMode);
    return os;
}

template <typename OStream>
inline OStream& operator<<(OStream& os, const DeviceConfig::FaceSelectionPolicy& policy)
{
    os << Description(policy);
    return os;
}
} // namespace RealSenseID
