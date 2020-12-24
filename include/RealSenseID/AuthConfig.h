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
    typedef enum RSID_API CameraRotation
    {
        Rotation_90_Deg = 0, // default
        Rotation_180_Deg = 1
    } CameraRotationType;
    CameraRotationType camera_rotation = Rotation_90_Deg;

    /* @enum
     * @brief SecurityLevel to allow
     */
    typedef enum RSID_API SecurityLevel
    {
        High = 0,  // default
        Medium = 1 // mask support
    } SecurityLevelType;
    SecurityLevelType security_level = High;
};

} // namespace RealSenseID
