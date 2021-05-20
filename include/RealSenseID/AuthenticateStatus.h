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
    CameraStarted,
    CameraStopped,
    MaskDetectedInHighSecurity,
    Spoof,
    Forbidden,
    DeviceError,
    Failure,
    /// serial statuses
    SerialOk = 100,
    SerialError,
    SerialSecurityError,
    VersionMismatch,
    CrcError,
    Reserved1 = 120,
    Reserved2,
    Reserved3
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
