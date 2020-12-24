// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "rsid_export.h"
#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        RSID_Serial_Ok = 100,
        RSID_Error,
        RSID_Serial_SecurityError
    } rsid_serial_status;

    typedef enum
    {
        RSID_Auth_Success,
        RSID_Auth_BadFrameQuality,
        RSID_Auth_NoFaceDetected,
        RSID_Auth_FaceDetected,
        RSID_Auth_LedFlowSuccess,
        RSID_Auth_FaceIsTooFarToTheTop,
        RSID_Auth_FaceIsTooFarToTheBottom,
        RSID_Auth_FaceIsTooFarToTheRight,
        RSID_Auth_FaceIsTooFarToTheLeft,
        RSID_Auth_FaceTiltIsTooUp,
        RSID_Auth_FaceTiltIsTooDown,
        RSID_Auth_FaceTiltIsTooRight,
        RSID_Auth_FaceTiltIsTooLeft,
        RSID_Auth_FaceIsTooFarFromTheCamera,
        RSID_Auth_FaceIsTooCloseToTheCamera,
        RSID_Auth_CameraStarted,
        RSID_Auth_CameraStopped,
        RSID_Auth_Forbidden,
        RSID_Auth_DeviceError,
        RSID_Auth_Failure,
        RSID_Auth_Serial_Ok = RSID_Serial_Ok,
        RSID_Auth_Serial_Error,
        RSID_Auth_Serial_SecurityError
    } rsid_auth_status;


    typedef enum
    {
        RSID_Enroll_Success,
        RSID_Enroll_BadFrameQuality,
        RSID_Enroll_NoFaceDetected,
        RSID_Enroll_FaceDetected,
        RSID_Enroll_LedFlowSuccess,
        RSID_Enroll_FaceIsTooFarToTheTop,
        RSID_Enroll_FaceIsTooFarToTheBottom,
        RSID_Enroll_FaceIsTooFarToTheRight,
        RSID_Enroll_FaceIsTooFarToTheLeft,
        RSID_Enroll_FaceTiltIsTooUp,
        RSID_Enroll_FaceTiltIsTooDown,
        RSID_Enroll_FaceTiltIsTooRight,
        RSID_Enroll_FaceTiltIsTooLeft,
        RSID_Enroll_FaceIsNotFrontal,
        RSID_Enroll_FaceIsTooFarFromTheCamera,
        RSID_Enroll_FaceIsTooCloseToTheCamera,
        RSID_Enroll_CameraStarted,
        RSID_Enroll_CameraStopped,
        RSID_Enroll_MultipleFacesDetected,
        RSID_Enroll_Failure,
        RSID_Enroll_DeviceError,
        RSID_Enroll_Serial_Ok = RSID_Serial_Ok,
        RSID_Enroll_Serial_Error,
        RSID_Enroll_Serial_SecurityError
    } rsid_enroll_status;


    typedef enum
    {
        RSID_Face_Center,
        RSID_Face_Up,
        RSID_Face_Down,
        RSID_Face_Left,
        RSID_Face_Right
    } rsid_face_pose;


    // c string representations of the statuses
    RSID_C_API const char* rsid_serial_status_str(rsid_serial_status status);
    RSID_C_API const char* rsid_auth_status_str(rsid_auth_status status);
    RSID_C_API const char* rsid_enroll_status_str(rsid_enroll_status status);
    RSID_C_API const char* rsid_face_pose_str(rsid_face_pose pose);

#ifdef __cplusplus
}
#endif