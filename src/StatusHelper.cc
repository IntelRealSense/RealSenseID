// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "StatusHelper.h"

namespace RealSenseID
{
const char* Description(Status status)
{
    switch (status)
    {
    case RealSenseID::Status::Ok:
        return "Ok";
    case RealSenseID::Status::Error:
        return "Error";
    case RealSenseID::Status::SerialError:
        return "SerialError";
    case RealSenseID::Status::SecurityError:
        return "SerialSecurityError";
    case RealSenseID::Status::VersionMismatch:
        return "VersionMismatch";
    default:
        return "Unknown Status";
    }
}

const char* Description(EnrollStatus status)
{
    // handle serial status
    if (status >= RealSenseID::EnrollStatus::SerialOk)
    {
        return Description(static_cast<RealSenseID::Status>(status));
    }

    switch (status)
    {
    case RealSenseID::EnrollStatus::Success:
        return "Success";
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
    case RealSenseID::EnrollStatus::SerialOk:
        return "SerialOk";
    case RealSenseID::EnrollStatus::SerialError:
        return "SerialError";
    case RealSenseID::EnrollStatus::SerialSecurityError:
        return "SerialSecurityError";
    case RealSenseID::EnrollStatus::VersionMismatch:
        return "VersionMismatch";
    case RealSenseID::EnrollStatus::Reserved1:
        return "Reserved1";
    case RealSenseID::EnrollStatus::Reserved2:
        return "Reserved2";
    case RealSenseID::EnrollStatus::Reserved3:
        return "Reserved3";
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
        return Description(static_cast<RealSenseID::Status>(status));
    }

    switch (status)
    {
    case RealSenseID::AuthenticateStatus::Success:
        return "Success";
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
    case RealSenseID::AuthenticateStatus::SerialOk:
        return "SerialOk";
    case RealSenseID::AuthenticateStatus::SerialError:
        return "SerialError";
    case RealSenseID::AuthenticateStatus::SerialSecurityError:
        return "SerialSecurityError";
    case RealSenseID::AuthenticateStatus::VersionMismatch:
        return "VersionMismatch";
    case RealSenseID::AuthenticateStatus::Reserved1:
        return "Reserved1";
    case RealSenseID::AuthenticateStatus::Reserved2:
        return "Reserved2";
    case RealSenseID::AuthenticateStatus::Reserved3:
        return "Reserved3";
    default:
        return "Unknown Status";
    }
}

const char* Description(AuthConfig::CameraRotation rotation)
{
    switch (rotation)
    {
    case AuthConfig::CameraRotation::Rotation_0_Deg:
        return "0 Degrees";
    case AuthConfig::CameraRotation::Rotation_180_Deg:
        return "180 Degrees";
    default:
        return "Unknown Value";
    }
}

const char* Description(AuthConfig::SecurityLevel level)
{
    switch (level)
    {
    case AuthConfig::SecurityLevel::High:
        return "High";
    case AuthConfig::SecurityLevel::Medium:
        return "Medium";
    default:
        return "Unknown value";
    }
}

EnrollStatus ToEnrollStatus(PacketManager::SerialStatus serial_status)
{
    switch (serial_status)
    {
    case PacketManager::SerialStatus::Ok:
        return EnrollStatus::SerialOk;
    case PacketManager::SerialStatus::SecurityError:
        return EnrollStatus::SerialSecurityError;
    case PacketManager::SerialStatus::VersionMismatch:
        return EnrollStatus::VersionMismatch;
    default:
        return EnrollStatus::SerialError;
    }
}

AuthenticateStatus ToAuthStatus(PacketManager::SerialStatus serial_status)
{
    switch (serial_status)
    {
    case PacketManager::SerialStatus::Ok:
        return AuthenticateStatus::SerialOk;
    case PacketManager::SerialStatus::SecurityError:
        return AuthenticateStatus::SerialSecurityError;
    case PacketManager::SerialStatus::VersionMismatch:
        return AuthenticateStatus::VersionMismatch;
    default:
        return AuthenticateStatus::SerialError;
    }
}

Status ToStatus(PacketManager::SerialStatus serial_status)
{
    switch (serial_status)
    {
    case PacketManager::SerialStatus::Ok:
        return Status::Ok;
    case PacketManager::SerialStatus::SecurityError:
        return Status::SecurityError;
    case PacketManager::SerialStatus::VersionMismatch:
        return Status::VersionMismatch;
    default:
        return Status::SerialError;
    }
}
} // namespace RealSenseID
