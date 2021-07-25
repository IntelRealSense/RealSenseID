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
        Rotation_270_Deg =3
    };

    /**
     * @enum SecurityLevel
     * @brief SecurityLevel to allow
     */
    enum class SecurityLevel
    {
        High = 0,   // high security, no mask support, all AS algo(s) will be activated
        Medium = 1, // default mode to support masks, projector wont be activated.
        Low = 2,    // low security level, only main AS algo will be activated. 
    };
    
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

    /**
     * @enum FaceSelectionPolicy
     * @brief Controls whether to run authentication on all (up to 5) detected faces vs single (closest) face
     */
    enum class FaceSelectionPolicy
    {
        Single = 0, // default, run authentication on closest face
        All = 1     // run authentication on all (up to 5) detected faces
    };

    enum class DumpMode
    {
        None = 0,
        CroppedFace = 1,
        FullFrame = 2,
    };

    // we allow 3 confidence levels. This is used in our Matcher during authentication :
    // each level means a different set of thresholds is used. 
    // This allow the user the flexibility to choose between 3 different FPR rates (Low, Medium, High).
    // Currently all sets are the "High" confidence level thresholds.
    enum class MatcherConfidenceLevel
    {
        High = 0,   // default
        Medium = 1, 
        Low = 2
    };

    CameraRotation camera_rotation = CameraRotation::Rotation_0_Deg;
    SecurityLevel security_level = SecurityLevel::Medium;
    AlgoFlow algo_flow = AlgoFlow::All;
    FaceSelectionPolicy face_selection_policy = FaceSelectionPolicy::Single;
    DumpMode dump_mode = DumpMode::None;
    MatcherConfidenceLevel matcher_confidence_level = MatcherConfidenceLevel::High;
};

RSID_API const char* Description(DeviceConfig::CameraRotation rotation);
RSID_API const char* Description(DeviceConfig::SecurityLevel level);
RSID_API const char* Description(DeviceConfig::AlgoFlow algoMode);
RSID_API const char* Description(DeviceConfig::FaceSelectionPolicy policy);
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
inline OStream& operator<<(OStream& os, const DeviceConfig::FaceSelectionPolicy& policy)
{
    os << Description(policy);
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
