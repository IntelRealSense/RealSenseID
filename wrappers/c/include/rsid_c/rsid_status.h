// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "rsid_export.h"
#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
    typedef enum
    {
        RSID_Rotation_0_Deg = 0, // default
        RSID_Rotation_180_Deg = 1,
        RSID_Rotation_90_Deg = 2,
        RSID_Rotation_270_Deg = 3
    } rsid_camera_rotation_type;

    typedef enum
    {
        RSID_SecLevel_High = 0,           // high security, no mask support, all AS algo(s) will be activated.
        RSID_SecLevel_Medium = 1,         // default mode to support masks, only main AS algo will be activated.
        RSID_SecLevel_Low = 2           // low device to run recognition only without AS.
    } rsid_security_level_type;

    // we allow 3 confidence levels. This is used in our Matcher during authentication :
    // each level means a different set of thresholds is used. 
    // This allow the user the flexibility to choose between 3 different FPR rates (Low, Medium, High).
    // Currently all sets are the "High" confidence level thresholds.
    typedef enum
    {
        RSID_MatcherConfLevel_High = 0,     // default      
        RSID_MatcherConfLevel_Medium = 1,   // 
        RSID_MatcherConfLevel_Low = 2       // 
    } rsid_matcher_confidence_level_type;

    typedef enum
    {
        RSID_AlgoMode_All = 0,            // default mode to run all algo(s)
        RSID_AlgoMode_SpoofOnly = 1,      // run Anti-Spoofing algo(s) only.
        RSID_AlgoMode_RecognitionOnly = 2 // configures device to run recognition only without AS.
    } rsid_algo_mode_type;

    typedef enum
    {
        RSID_FacePolicy_Single = 0, // default, run authentication on closest face
        RSID_FacePolicy_All = 1,    // run authentication on all (up to 5) detected faces
    } rsid_face_policy_type;

    typedef enum
    {
        MJPEG_1080P = 0,
        MJPEG_720P = 1,
        RAW10_1080P = 2,
    } rsid_preview_mode;

    typedef enum
    {
        RISD_DumpNone = 0,
        RSID_DumpCroppedFace = 1,
        RSID_DumpFullFrame = 2,
    } rsid_dump_mode;

    typedef enum
    {
        RSID_Ok = 100,
        RSID_Error,
        RSID_SerialError,
        RSID_SecurityError,
        RSID_VersionMismatch,
        RSID_CrcError,
        RSID_LicenseError,
        RSID_LicenseCheck,
        RSID_TooManySpoofs
    } rsid_status;

    typedef enum
    {
        RSID_Auth_Success,
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
        RSID_Auth_CameraStarted,
        RSID_Auth_CameraStopped,
        RSID_Auth_MaskDetectedInHighSecurity,
        RSID_Auth_Spoof,
        RSID_Auth_Forbidden,
        RSID_Auth_DeviceError,
        RSID_Auth_Failure,
        RSID_Auth_TooManySpoofs,
        RSID_Auth_InvalidFeatures,
        RSID_Auth_Serial_Ok = RSID_Ok,
        RSID_Auth_Serial_Error,
        RSID_Auth_Serial_SerialError,
        RSID_Auth_Serial_SecurityError,
        RSID_Auth_Serial_VersionMismatch,
        RSID_Auth_Serial_CrcError,
        RSID_Auth_Serial_LicenseError,
        RSID_Auth_Serial_LicenseCheck,
        RSID_Auth_Spoof_2D = 120,
        RSID_Auth_Spoof_3D,
        RSID_Auth_Spoof_LR,
        RSID_Auth_Spoof_Disparity
    } rsid_auth_status;

    typedef enum
    {
        RSID_Enroll_Success,
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
        RSID_Enroll_CameraStarted,
        RSID_Enroll_CameraStopped,
        RSID_Enroll_MultipleFacesDetected,
        RSID_Enroll_Failure,
        RSID_Enroll_DeviceError,
        RSID_EnrollWithMaskIsForbidden,
        RSID_Enroll_Spoof,
        RSID_Enroll_InvalidFeatures,
        RSID_Enroll_Serial_Ok = RSID_Ok,
        RSID_Enroll_Serial_Error,
        RSID_Enroll_Serial_SerialError,
        RSID_Enroll_Serial_SecurityError,
        RSID_Enroll_Serial_VersionMismatch,
        RSID_Enroll_Serial_CrcError,
        RSID_Enroll_Serial_LicenseError,
        RSID_Enroll_Serial_LicenseCheck,
        RSID_Enroll_Spoof_2D = 120,
        RSID_Enroll_Spoof_3D,
        RSID_Enroll_Spoof_LR,
        RSID_Enroll_Spoof_Disparity
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

    typedef enum
    {
        RSID_Continous,
        RSID_Opfw_First,
        RSID_Require_Intermediate_Fw,
        RSID_Not_Allowed
    } rsid_update_policy;

    typedef struct rsid_match_result
    {
        int success;
        int should_update;
        int score;
    } rsid_match_result;

    // c string representations of the statuses
    RSID_C_API const char* rsid_status_str(rsid_status status);
    RSID_C_API const char* rsid_auth_status_str(rsid_auth_status status);
    RSID_C_API const char* rsid_enroll_status_str(rsid_enroll_status status);
    RSID_C_API const char* rsid_face_pose_str(rsid_face_pose pose);
    RSID_C_API const char* rsid_auth_settings_rotation(rsid_camera_rotation_type rotation);
    RSID_C_API const char* rsid_auth_settings_level(rsid_security_level_type level);
    RSID_C_API const char* rsid_auth_settings_algo_mode(rsid_algo_mode_type mode);

#ifdef __cplusplus
}
#endif //__cplusplus