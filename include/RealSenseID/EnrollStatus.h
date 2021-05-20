// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseIDExports.h"

namespace RealSenseID
{
/**
 * Statuses returned from FaceAuthenticator enroll operation.
 */
enum class RSID_API EnrollStatus
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
    MultipleFacesDetected,    
    Failure,
    DeviceError,
    EnrollWithMaskIsForbidden,  // for mask-detector : we'll forbid enroll if used wears mask.
    Spoof,
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
RSID_API const char* Description(EnrollStatus status);

template <typename OStream>
inline OStream& operator<<(OStream& os, const EnrollStatus& status)
{
    os << Description(status);
    return os;
}
} // namespace RealSenseID
