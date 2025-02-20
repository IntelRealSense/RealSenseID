// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseIDExports.h"

namespace RealSenseID
{
/**
 * Statuses returned from FaceAuthenticator authenticate operation.
 */
enum class RSID_API AuthenticateStatus
{
    Success,
    NoFaceDetected,
    FaceDetected,
    LedFlowSuccess,
    FaceIsTooFarToTheTop,
    FaceIsTooFarToTheBottom,
    FaceIsTooFarToTheRight,
    FaceIsTooFarToTheLeft,
    FaceTiltIsTooUp,
    FaceTiltIsTooDown,
    FaceTiltIsTooRight,
    FaceTiltIsTooLeft,
    FaceIsNotFrontal,
    CameraStarted,
    CameraStopped,
    MaskDetectedInHighSecurity,
    Spoof,
    Forbidden,
    DeviceError,
    Failure,
    TooManySpoofs,
    InvalidFeatures,
    AmbiguiousFace,
    /// Accessories
    Sunglasses = 50,
    /// serial statuses
    Ok = 100,
    Error,
    SerialError,
    SecurityError,
    VersionMismatch,
    CrcError,
    LicenseError,
    LicenseCheck,
    /// Spoofs
    Spoof_2D = 120,
    Spoof_3D,
    Spoof_LR,
    Spoof_Disparity,
    Spoof_Surface,
    Spoof_Plane_Disparity,
    Spoof_2D_Right,
    /* Note: Should not exceed 127 - to be a legal ascii*/
};

/**
 * Return c string description of the status
 *
 * @param status to describe.
 */
RSID_API const char* Description(AuthenticateStatus status);

template <typename OStream>
inline OStream& operator<<(OStream& os, const AuthenticateStatus& status)
{
    os << Description(status);
    return os;
}
} // namespace RealSenseID
