// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "rsid_export.h"
#include "rsid_status.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        RSID_USB,
        RSID_UART
    } rsid_serial_type;

    typedef struct
    {
        rsid_serial_type serial_type;
        const char* port;
    } rsid_serial_config;

    typedef enum
    {
        ROTATION_90_DEG = 0, // default
        ROTATION_180_DEG
    } rsid_camera_rotation_type;

    typedef enum
    {
        HIGH = 0,  // default
        MEDIUM = 1 // mask support
    } rsid_security_level_type;


    typedef struct
    {
        rsid_camera_rotation_type camera_rotation;
        rsid_security_level_type security_level;
    } rsid_auth_config;

    typedef struct
    {
        void* _impl;
    } rsid_authenticator;


    /*
     * User defined callback to sign a given buffer before it is sent to the device.
     * Sign the buffer and copy the signature to the out_sig buffer (64 bytes)
     * Return 1 if succeeded and 0 otherwise.
     */
    typedef int (*rsid_sign_clbk)(const unsigned char* buffer, const unsigned int buffer_len, unsigned char* out_sig,
                                  void* ctx);

    /*
     * User defined callback to verify the buffer and the given signature.
     * Return 1 if succeeded and verified, 0 otherwise.
     */
    typedef int (*rsid_verify_clbk)(const unsigned char* buffer, const unsigned int buffer_len,
                                    const unsigned char* sig, const unsigned int siglen, void* ctx);

    typedef struct rsid_signature_clbk
    {
        rsid_sign_clbk sign_clbk;     /* user defined sign clbk */
        rsid_verify_clbk verify_clbk; /* user defined verify clbk */
        void* ctx;                    /* user defined context (optional, set to NULL if not needed) */
    } rsid_signature_clbk;

    /* enroll callbacks */
    typedef void (*rsid_auth_status_clbk)(rsid_auth_status status, const char* user_id, void* ctx);
    typedef void (*rsid_auth_hint_clbk)(rsid_auth_status hint, void* ctx);

    /* authenticate callbacks */
    typedef void (*rsid_enroll_status_clbk)(rsid_enroll_status status, void* ctx);
    typedef void (*rsid_enroll_progress_clbk)(rsid_face_pose face_pose, void* ctx);
    typedef void (*rsid_enroll_hint_clbk)(rsid_enroll_status hint, void* ctx);

    /* rsid_authenticate() args */
    typedef struct rsid_auth_args
    {
        rsid_auth_status_clbk result_clbk; /* result callback */
        rsid_auth_hint_clbk hint_clbk;     /* hint callback */
        void* ctx;                         /* user defined context (optional) */
    } rsid_auth_args;

    /* rsid_enroll() args */
    typedef struct rsid_enroll_args
    {
        const char* user_id; /* user id. null terminated string of ascii chars (max 16 chars + 1 terminating null) */
        rsid_enroll_status_clbk status_clbk;     /* status callback */
        rsid_enroll_progress_clbk progress_clbk; /* progress callback */
        rsid_enroll_hint_clbk hint_clbk;         /* hint calback */
        void* ctx;                               /* user defined context (optional, set to null if not needed) */
    } rsid_enroll_args;

    /* return new authenticator pointer (or null on failure) */
    RSID_C_API rsid_authenticator* rsid_create_authenticator(rsid_signature_clbk* signature_clbk);

    /* destroy the authenticator and free its resources */
    RSID_C_API void rsid_destroy_authenticator(rsid_authenticator* authenticator);

    /* connect */
    RSID_C_API rsid_serial_status rsid_connect(rsid_authenticator* authenticator,
                                               const rsid_serial_config* client_config);

    /* set advanced settings to FW */
    RSID_C_API rsid_serial_status rsid_set_auth_settings(rsid_authenticator* authenticator,
                                                         const rsid_auth_config* auth_config);

    /* disconnect */
    RSID_C_API void rsid_disconnect(rsid_authenticator* authenticator);

    /* enroll a user */
    RSID_C_API rsid_enroll_status rsid_enroll(rsid_authenticator* authenticator, const rsid_enroll_args* args);

    /* authenticate a user */
    RSID_C_API rsid_auth_status rsid_authenticate(rsid_authenticator* authenticator, const rsid_auth_args* args);

    /* authenticate in an infinite loop until rsid_cancel is called */
    RSID_C_API rsid_auth_status rsid_authenticate_loop(rsid_authenticator* authenticator, const rsid_auth_args* args);

    /* authenticate in an infinite loop until cancel is called */
    RSID_C_API rsid_serial_status rsid_cancel(rsid_authenticator* authenticator);

    /* remove given user the the device */
    RSID_C_API rsid_serial_status rsid_remove_user(rsid_authenticator* authenticator, const char* user_id);

    /* remove all users from the device */
    RSID_C_API rsid_serial_status rsid_remove_all_users(rsid_authenticator* authenticator);

    /* return library version */
    RSID_C_API const char* rsid_version();

#ifdef __cplusplus
}
#endif
