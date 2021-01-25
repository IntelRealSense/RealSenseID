// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

namespace RealSenseID
{
struct RSID_API AuthConfig
{
    /* @enum
     * @brief Camera rotation.
     */
    enum class CameraRotation
    {
        Rotation_0_Deg = 0, // default
        Rotation_180_Deg = 1
    };
    CameraRotation camera_rotation = CameraRotation::Rotation_0_Deg;

    /* @enum
     * @brief SecurityLevel to allow
     */
    enum class SecurityLevel {
        High = 0,  // default
        Medium = 1 // mask support
    };
    SecurityLevel security_level = SecurityLevel::High;
};

RSID_API const char* Description(AuthConfig::CameraRotation rotation);
RSID_API const char* Description(AuthConfig::SecurityLevel level);

template <typename OStream>
inline OStream& operator<<(OStream& os, const AuthConfig::CameraRotation& rotation)
{
    os << Description(rotation);
    return os;
}

template <typename OStream>
inline OStream& operator<<(OStream& os, const AuthConfig::SecurityLevel& level)
{
    os << Description(level);
    return os;
}


} // namespace RealSenseID
