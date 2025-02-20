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
        return "SecurityError";
    case RealSenseID::Status::VersionMismatch:
        return "VersionMismatch";
    case RealSenseID::Status::CrcError:
        return "CrcError";
    case RealSenseID::Status::LicenseError:
        return "LicenseError";
    case RealSenseID::Status::LicenseCheck:
        return "LicenseCheck";
    case RealSenseID::Status::TooManySpoofs:
        return "TooManySpoofs";
    default:
        return "Unknown Status";
    }
}

const char* Description(EnrollStatus status)
{
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
        return "FaceTiltIsTooUp";
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
    case RealSenseID::EnrollStatus::Ok:
        return "Ok";
    case RealSenseID::EnrollStatus::Error:
        return "Error";
    case RealSenseID::EnrollStatus::SerialError:
        return "SerialError";
    case RealSenseID::EnrollStatus::SecurityError:
        return "SecurityError";
    case RealSenseID::EnrollStatus::VersionMismatch:
        return "VersionMismatch";
    case RealSenseID::EnrollStatus::CrcError:
        return "CrcError";
    case RealSenseID::EnrollStatus::LicenseError:
        return "LicenseError";
    case RealSenseID::EnrollStatus::LicenseCheck:
        return "LicenseCheck";
    case RealSenseID::EnrollStatus::Spoof:
        return "Spoof";
    case RealSenseID::EnrollStatus::Spoof_2D:
        return "Spoof_2D";
    case RealSenseID::EnrollStatus::Spoof_3D:
        return "Spoof_3D";
    case RealSenseID::EnrollStatus::Spoof_LR:
        return "Spoof_LR";
    case RealSenseID::EnrollStatus::Spoof_Surface:
        return "Spoof_Surface";
    case RealSenseID::EnrollStatus::Spoof_Disparity:
        return "Spoof_Disparity";
    case RealSenseID::EnrollStatus::InvalidFeatures:
        return "Invalid_Features";
    case RealSenseID::EnrollStatus::Spoof_2D_Right:
        return "Spoof_2D_Right";
    case RealSenseID::EnrollStatus::Spoof_Plane_Disparity:
        return "Spoof_Plane_Disparity";
    case RealSenseID::EnrollStatus::AmbiguiousFace:
        return "Ambiguious_Face";
    case RealSenseID::EnrollStatus::Sunglasses:
        return "Sunglasses";
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
    case RealSenseID::AuthenticateStatus::FaceIsNotFrontal:
        return "FaceIsNotFrontal";
    case RealSenseID::AuthenticateStatus::CameraStarted:
        return "CameraStarted";
    case RealSenseID::AuthenticateStatus::CameraStopped:
        return "CameraStopped";
    case RealSenseID::AuthenticateStatus::MaskDetectedInHighSecurity:
        return "MaskDetectedInHighSecurity";
    case RealSenseID::AuthenticateStatus::Spoof:
        return "Spoof";
    case RealSenseID::AuthenticateStatus::Forbidden:
        return "Forbidden";
    case RealSenseID::AuthenticateStatus::DeviceError:
        return "DeviceError";
    case RealSenseID::AuthenticateStatus::Failure:
        return "Failure";
    case RealSenseID::AuthenticateStatus::Ok:
        return "Ok";
    case RealSenseID::AuthenticateStatus::Error:
        return "Error";
    case RealSenseID::AuthenticateStatus::SerialError:
        return "SerialError";
    case RealSenseID::AuthenticateStatus::SecurityError:
        return "SecurityError";
    case RealSenseID::AuthenticateStatus::VersionMismatch:
        return "VersionMismatch";
    case RealSenseID::AuthenticateStatus::CrcError:
        return "CrcError";
    case RealSenseID::AuthenticateStatus::LicenseError:
        return "LicenseError";
    case RealSenseID::AuthenticateStatus::LicenseCheck:
        return "LicenseCheck";
    case RealSenseID::AuthenticateStatus::Spoof_2D:
        return "Spoof_2D";
    case RealSenseID::AuthenticateStatus::Spoof_3D:
        return "Spoof_3D";
    case RealSenseID::AuthenticateStatus::Spoof_LR:
        return "Spoof_LR";
    case RealSenseID::AuthenticateStatus::Spoof_Surface:
        return "Spoof_Surface";
    case RealSenseID::AuthenticateStatus::Spoof_Disparity:
        return "Spoof_Disparity";
    case RealSenseID::AuthenticateStatus::TooManySpoofs:
        return "TooManySpoofs";
    case RealSenseID::AuthenticateStatus::InvalidFeatures:
        return "Invalid_Features";
    case RealSenseID::AuthenticateStatus::Spoof_2D_Right:
        return "Spoof_2D_Right";
    case RealSenseID::AuthenticateStatus::Spoof_Plane_Disparity:
        return "Spoof_Plane_Disparity";
    case RealSenseID::AuthenticateStatus::AmbiguiousFace:
        return "Ambiguious_Face";
    case RealSenseID::AuthenticateStatus::Sunglasses:
        return "Sunglasses";
    default:
        return "Unknown Status";
    }
}

const char* Description(DeviceConfig::CameraRotation rotation)
{
    switch (rotation)
    {
    case DeviceConfig::CameraRotation::Rotation_0_Deg:
        return "0 Degrees";
    case DeviceConfig::CameraRotation::Rotation_180_Deg:
        return "180 Degrees";
    default:
        return "Unknown Value";
    }
}

const char* Description(DeviceConfig::AlgoFlow algo_flow)
{
    switch (algo_flow)
    {
    case DeviceConfig::AlgoFlow::All:
        return "All";
    case DeviceConfig::AlgoFlow::RecognitionOnly:
        return "RecognitionOnly";
    case DeviceConfig::AlgoFlow::SpoofOnly:
        return "SpoofOnly";
    case DeviceConfig::AlgoFlow::FaceDetectionOnly:
        return "FaceDetectionOnly";
    default:
        return "Unknown Value";
    }
}

const char* Description(DeviceConfig::SecurityLevel level)
{
    switch (level)
    {
    case DeviceConfig::SecurityLevel::High:
        return "High";
    case DeviceConfig::SecurityLevel::Medium:
        return "Medium";
    case DeviceConfig::SecurityLevel::Low:
        return "Low";
    default:
        return "Unknown value";
    }
}

const char* Description(DeviceConfig::MatcherConfidenceLevel matcher_confidence_level)
{
    switch (matcher_confidence_level)
    {
    case DeviceConfig::MatcherConfidenceLevel::High:
        return "High";
    case DeviceConfig::MatcherConfidenceLevel::Medium:
        return "Medium";
    case DeviceConfig::MatcherConfidenceLevel::Low:
        return "Low";
    default:
        return "Unknown value";
    }
}


const char* Description(DeviceConfig::DumpMode dump_mode)
{
    switch (dump_mode)
    {
    case DeviceConfig::DumpMode::None:
        return "None";
    case DeviceConfig::DumpMode::CroppedFace:
        return "CroppedFace";
    case DeviceConfig::DumpMode::FullFrame:
        return "FullFrame";
    default:
        return "Unknown value";
    }
}

EnrollStatus ToEnrollStatus(PacketManager::SerialStatus serial_status)
{
    switch (serial_status)
    {
    case PacketManager::SerialStatus::Ok:
        return EnrollStatus::Ok;
    case PacketManager::SerialStatus::SecurityError:
        return EnrollStatus::SecurityError;
    case PacketManager::SerialStatus::VersionMismatch:
        return EnrollStatus::VersionMismatch;
    case PacketManager::SerialStatus::CrcError:
        return EnrollStatus::CrcError;
    case PacketManager::SerialStatus::LicenseError:
        return EnrollStatus::LicenseError;
    case PacketManager::SerialStatus::LicenseCheck:
        return EnrollStatus::LicenseCheck;
    default:
        return EnrollStatus::Error;
    }
}

AuthenticateStatus ToAuthStatus(PacketManager::SerialStatus serial_status)
{
    switch (serial_status)
    {
    case PacketManager::SerialStatus::Ok:
        return AuthenticateStatus::Ok;
    case PacketManager::SerialStatus::SecurityError:
        return AuthenticateStatus::SecurityError;
    case PacketManager::SerialStatus::VersionMismatch:
        return AuthenticateStatus::VersionMismatch;
    case PacketManager::SerialStatus::CrcError:
        return AuthenticateStatus::CrcError;
    case PacketManager::SerialStatus::LicenseError:
        return AuthenticateStatus::LicenseError;
    case PacketManager::SerialStatus::LicenseCheck:
        return AuthenticateStatus::LicenseCheck;
    default:
        return AuthenticateStatus::Error;
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
    case PacketManager::SerialStatus::CrcError:
        return Status::CrcError;
    case PacketManager::SerialStatus::LicenseError:
        return Status::LicenseError;
    case PacketManager::SerialStatus::LicenseCheck:
        return Status::LicenseCheck;
    default:
        return Status::Error;
    }
}

PacketManager::SerialStatus ToSerialStatus(Status fa_status)
{
    switch (fa_status)
    {
    case Status::Ok:
        return PacketManager::SerialStatus::Ok;
    case Status::Error:
        return PacketManager::SerialStatus::SecurityError;
    case Status::SerialError:
        return PacketManager::SerialStatus::RecvFailed;
    case Status::SecurityError:
        return PacketManager::SerialStatus::SecurityError;
    case Status::VersionMismatch:
        return PacketManager::SerialStatus::VersionMismatch;
    case Status::CrcError:
        return PacketManager::SerialStatus::CrcError;
    case Status::LicenseError:
        return PacketManager::SerialStatus::LicenseError;
    case Status::LicenseCheck:
        return PacketManager::SerialStatus::LicenseCheck;
    default:
        return PacketManager::SerialStatus::SecurityError;
    }
}
} // namespace RealSenseID
