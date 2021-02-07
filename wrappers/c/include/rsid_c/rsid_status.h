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
        RSID_USB,
        RSID_UART
    } rsid_serial_type;


    typedef enum
    {
        RSID_Rotation_0_Deg = 0, // default
        RSID_Rotation_180_Deg
    } rsid_camera_rotation_type;

    typedef enum
    {
        RSID_SecLevel_High = 0, // default
        RSID_SecLevel_Medium = 1  // mask support
    } rsid_security_level_type;


    typedef enum
    {
        RSID_Ok = 100,
        RSID_Error,
        RSID_SerialError,
        RSID_SecurityError,
        RSID_VersionMismatch
    } rsid_status;

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
        RSID_Auth_Serial_Ok = RSID_Ok,
        RSID_Auth_Serial_Error,
        RSID_Auth_Serial_SecurityError,
        RSID_Auth_Serial_VersionMismatch
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
        RSID_Enroll_Serial_Ok = RSID_Ok,
        RSID_Enroll_Serial_Error,
        RSID_Enroll_Serial_SecurityError,
        RSID_Enroll_Serial_VersionMismatch
    } rsid_enroll_status;


    typedef enum
    {
        RSID_Face_Center,
        RSID_Face_Up,
        RSID_Face_Down,
        RSID_Face_Left,
        RSID_Face_Right
    } rsid_face_pose;

      /* log callback support*/
    typedef enum
    {
        RSID_LogLevel_Trace,
        RSID_LogLevel_Debug,
        RSID_LogLevel_Info,
        RSID_LogLevel_Warning,
        RSID_LogLevel_Error,
        RSID_LogLevel_Critical,
        RSID_LogLevel_Off
    } rsid_log_level;


    typedef struct rsid_match_result
    {
        int success;
        int should_update;        
    } rsid_match_result;
	
    // c string representations of the statuses
    RSID_C_API const char* rsid_status_str(rsid_status status);
    RSID_C_API const char* rsid_auth_status_str(rsid_auth_status status);
    RSID_C_API const char* rsid_enroll_status_str(rsid_enroll_status status);
    RSID_C_API const char* rsid_face_pose_str(rsid_face_pose pose);
    RSID_C_API const char* rsid_auth_settings_rotation(rsid_camera_rotation_type rotation);    
    RSID_C_API const char* rsid_auth_settings_level(rsid_security_level_type level);

#ifdef __cplusplus
}
#endif