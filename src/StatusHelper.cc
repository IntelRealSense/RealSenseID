// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "StatusHelper.h"

namespace RealSenseID
{
const char* Description(SerialStatus status)
{
    switch (status)
    {
    case RealSenseID::SerialStatus::Ok:
        return "Ok";
    case RealSenseID::SerialStatus::Error:
        return "SerialError";
    case RealSenseID::SerialStatus::SecurityError:
        return "SerialSecurityError";
    default:
        return "Unknown Status";
    }
}

const char* Description(EnrollStatus status)
{
    // handle serial status
    if (status >= RealSenseID::EnrollStatus::SerialOk)
    {
        return Description(static_cast<RealSenseID::SerialStatus>(status));
    }

    switch (status)
    {
    case RealSenseID::EnrollStatus::Success:
        return "Success";
    case RealSenseID::EnrollStatus::BadFrameQuality:
        return "BadFrameQuality";
    case RealSenseID::EnrollStatus::NoFaceDetected:
        return "NoFaceDetected";
    case RealSenseID::EnrollStatus::FaceDetected:
        return "FaceDetected";
    case RealSenseID::EnrollStatus::LedFlowSuccess:
        return "LedFlowSuccess";
    case RealSenseID::EnrollStatus::FaceIsTooFarToTheTop:
        return "FaceIsTooFarToTheTop";
    case RealSenseID::EnrollStatus::FaceIsTooFarToTheBottom:
        return "FaceIsTooFarToTheBottom";
    case RealSenseID::EnrollStatus::FaceIsTooFarToTheRight:
        return "FaceIsTooFarToTheRight";
    case RealSenseID::EnrollStatus::FaceIsTooFarToTheLeft:
        return "FaceIsTooFarToTheLeft";
    case RealSenseID::EnrollStatus::FaceTiltIsTooUp:
        return "Success";
    case RealSenseID::EnrollStatus::FaceTiltIsTooDown:
        return "FaceTiltIsTooDown";
    case RealSenseID::EnrollStatus::FaceTiltIsTooRight:
        return "FaceTiltIsTooRight";
    case RealSenseID::EnrollStatus::FaceTiltIsTooLeft:
        return "FaceTiltIsTooLeft";
    case RealSenseID::EnrollStatus::FaceIsNotFrontal:
        return "FaceIsNotFrontal";
    case RealSenseID::EnrollStatus::FaceIsTooFarFromTheCamera:
        return "FaceIsTooFarFromTheCamera";
    case RealSenseID::EnrollStatus::FaceIsTooCloseToTheCamera:
        return "FaceIsTooCloseToTheCamera";
    case RealSenseID::EnrollStatus::CameraStarted:
        return "CameraStarted";
    case RealSenseID::EnrollStatus::CameraStopped:
        return "CameraStopped";
    case RealSenseID::EnrollStatus::MultipleFacesDetected:
        return "MultipleFacesDetected";
    case RealSenseID::EnrollStatus::Failure:
        return "Failure";
    case RealSenseID::EnrollStatus::DeviceError:
        return "DeviceError";
    default:
        return "Unknown Status";
    }
}


const char* Description(FacePose pose)
{
    switch (pose)
    {
    case RealSenseID::FacePose::Center:
        return "Center";
    case RealSenseID::FacePose::Up:
        return "Up";
    case RealSenseID::FacePose::Down:
        return "Down";
    case RealSenseID::FacePose::Left:
        return "Left";
    case RealSenseID::FacePose::Right:
        return "Right";
    default:
        return "Unknown Pose";
    }
}

const char* Description(AuthenticateStatus status)
{
    if (status >= RealSenseID::AuthenticateStatus::SerialOk)
    {
        return Description(static_cast<RealSenseID::SerialStatus>(status));
    }

    switch (status)
    {
    case RealSenseID::AuthenticateStatus::Success:
        return "Success";
    case RealSenseID::AuthenticateStatus::BadFrameQuality:
        return "BadFrameQuality";
    case RealSenseID::AuthenticateStatus::NoFaceDetected:
        return "NoFaceDetected";
    case RealSenseID::AuthenticateStatus::FaceDetected:
        return "FaceDetected";
    case RealSenseID::AuthenticateStatus::LedFlowSuccess:
        return "LedFlowSuccess";
    case RealSenseID::AuthenticateStatus::FaceIsTooFarToTheTop:
        return "FaceIsTooFarToTheTop";
    case RealSenseID::AuthenticateStatus::FaceIsTooFarToTheBottom:
        return "FaceIsTooFarToTheBottom";
    case RealSenseID::AuthenticateStatus::FaceIsTooFarToTheRight:
        return "FaceIsTooFarToTheRight";
    case RealSenseID::AuthenticateStatus::FaceIsTooFarToTheLeft:
        return "Success";
    case RealSenseID::AuthenticateStatus::FaceTiltIsTooUp:
        return "FaceTiltIsTooUp";
    case RealSenseID::AuthenticateStatus::FaceTiltIsTooDown:
        return "FaceTiltIsTooDown";
    case RealSenseID::AuthenticateStatus::FaceTiltIsTooRight:
        return "FaceTiltIsTooRight";
    case RealSenseID::AuthenticateStatus::FaceTiltIsTooLeft:
        return "FaceTiltIsTooLeft";
    case RealSenseID::AuthenticateStatus::FaceIsTooFarFromTheCamera:
        return "FaceIsTooFarFromTheCamera";
    case RealSenseID::AuthenticateStatus::FaceIsTooCloseToTheCamera:
        return "FaceIsTooCloseToTheCamera";
    case RealSenseID::AuthenticateStatus::CameraStarted:
        return "CameraStarted";
    case RealSenseID::AuthenticateStatus::CameraStopped:
        return "CameraStopped";
    case RealSenseID::AuthenticateStatus::Forbidden:
        return "Forbidden";
    case RealSenseID::AuthenticateStatus::DeviceError:
        return "DeviceError";
    case RealSenseID::AuthenticateStatus::Failure:
        return "Failure";
    default:
        return "Unknown Status";
    }
}

EnrollStatus ToEnrollStatus(PacketManager::Status serial_status)
{
    switch (serial_status)
    {
    case PacketManager::Status::Ok:
        return EnrollStatus::SerialOk;
    case PacketManager::Status::SecurityError:
        return EnrollStatus::SerialSecurityError;
    default:
        return EnrollStatus::SerialError;
    }
}

AuthenticateStatus ToAuthStatus(PacketManager::Status serial_status)
{
    switch (serial_status)
    {
    case PacketManager::Status::Ok:
        return AuthenticateStatus::SerialOk;
    case PacketManager::Status::SecurityError:
        return AuthenticateStatus::SerialSecurityError;
    default:
        return AuthenticateStatus::SerialError;
    }
}

SerialStatus ToSerialStatus(PacketManager::Status serial_status)
{
    switch (serial_status)
    {
    case PacketManager::Status::Ok:
        return SerialStatus::Ok;
    case PacketManager::Status::SecurityError:
        return SerialStatus::SecurityError;
    default:
        return SerialStatus::Error;
    }
}


} // namespace RealSenseID
