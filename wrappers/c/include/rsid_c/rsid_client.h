// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "rsid_export.h"
#include "rsid_status.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        rsid_serial_type serial_type;
        const char* port;
    } rsid_serial_config;

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

    /* rsid_authenticate() args */
    typedef void (*rsid_auth_status_clbk)(rsid_auth_status status, const char* user_id, void* ctx);
    typedef void (*rsid_auth_hint_clbk)(rsid_auth_status hint, void* ctx);

    typedef struct rsid_auth_args
    {
        rsid_auth_status_clbk result_clbk; /* result callback */
        rsid_auth_hint_clbk hint_clbk;     /* hint callback */
        void* ctx;                         /* user defined context (optional) */
    } rsid_auth_args;

    /* rsid_enroll() args */
    typedef void (*rsid_enroll_status_clbk)(rsid_enroll_status status, void* ctx);
    typedef void (*rsid_enroll_progress_clbk)(rsid_face_pose face_pose, void* ctx);
    typedef void (*rsid_enroll_hint_clbk)(rsid_enroll_status hint, void* ctx);
    typedef struct rsid_enroll_args
    {
        const char* user_id; /* user id. null terminated c string of ascii chars (max 15 chars + 1 terminating null) */
        rsid_enroll_status_clbk status_clbk;     /* status callback */
        rsid_enroll_progress_clbk progress_clbk; /* progress callback */
        rsid_enroll_hint_clbk hint_clbk;         /* hint calback */
        void* ctx;                               /* user defined context (optional, set to null if not needed) */
    } rsid_enroll_args;

    /* log callback */
    typedef void (*rsid_log_clbk)(rsid_log_level log_level, const char* msg);

    /* return new authenticator pointer (or null on failure) */
    RSID_C_API rsid_authenticator* rsid_create_authenticator(rsid_signature_clbk* signature_clbk);

    /* destroy the authenticator and free its resources */
    RSID_C_API void rsid_destroy_authenticator(rsid_authenticator* authenticator);

    /* connect */
    RSID_C_API rsid_status rsid_connect(rsid_authenticator* authenticator, const rsid_serial_config* serial_config);

    /* disconnect */
    RSID_C_API void rsid_disconnect(rsid_authenticator* authenticator);

    /* send updated host ecdsa key to device, sign it with previous ecdsa key (at first pair can sign with dummy key)
     * output is device's ecdsa key
     */
    typedef struct rsid_pairing_args
    {
        char host_pubkey[64];          /* input host public key*/
        char host_pubkey_sig[32];      /* input host public key signature */
        char device_pubkey_result[64]; /* output device's public key*/
    } rsid_pairing_args;

    RSID_C_API rsid_status rsid_pair(rsid_authenticator* authenticator, rsid_pairing_args* pairing_args);

    /* set advanced settings to FW */
    RSID_C_API rsid_status rsid_set_auth_settings(rsid_authenticator* authenticator,
                                                  const rsid_auth_config* auth_config);

    /* get advanced settings from FW */
    RSID_C_API rsid_status rsid_query_auth_settings(rsid_authenticator* authenticator, rsid_auth_config* auth_config);

    /* enroll a user */
    RSID_C_API rsid_status rsid_enroll(rsid_authenticator* authenticator, const rsid_enroll_args* args);

    /* authenticate a user */
    RSID_C_API rsid_status rsid_authenticate(rsid_authenticator* authenticator, const rsid_auth_args* args);

    /* authenticate in an infinite loop until rsid_cancel is called */
    RSID_C_API rsid_status rsid_authenticate_loop(rsid_authenticator* authenticator, const rsid_auth_args* args);

    /* authenticate in an infinite loop until cancel is called */
    RSID_C_API rsid_status rsid_cancel(rsid_authenticator* authenticator);

    /* remove given user the the device */
    RSID_C_API rsid_status rsid_remove_user(rsid_authenticator* authenticator, const char* user_id);

    /* remove all users from the device */
    RSID_C_API rsid_status rsid_remove_all_users(rsid_authenticator* authenticator);

    /* return library version */
    RSID_C_API const char* rsid_version();

    /* set log callback to be called when log with at least min_level is available */
    RSID_C_API void rsid_set_log_clbk(rsid_log_clbk clbk, rsid_log_level min_level, int do_formatting);


    /*
     * Query ids of all enrolled users from device.
     * On successfull operation, the result copied into the user_ids array and number_of_users is updated accordingly.
     * Note: char **user_ids should be a fully allocated array of user ids (each user id is char[16]).
     *       Use rsid_query_number_of_users(..) to find out how many slots to allocate in the array.
     */
    RSID_C_API rsid_status rsid_query_user_ids(rsid_authenticator* authenticator, char** user_ids,
                                               unsigned int* number_of_users);


    /*
     * Query ids of all enrolled users from device.
     * On successfull operation, the result copied into the result_buf and number_of_users is updated accordingly.
     * The result buf will contain all user ids (16 byte chunks).
     * Note: result_buf must be allocted with size of at least (number_of_users * 16)
     */

    RSID_C_API rsid_status rsid_query_user_ids_to_buf(rsid_authenticator* authenticator, char* result_buf,
                                                      unsigned int* number_of_users);

    /*
     * Get number of enrolled users from device.
     * On successfull operation, the result is placed in number_of_users.
     */
    RSID_C_API rsid_status rsid_query_number_of_users(rsid_authenticator* authenticator, unsigned int* number_of_users);

    /* Prepare device to stadby */
    RSID_C_API rsid_status rsid_standby(rsid_authenticator* authenticator);

    /*
     * device controller functions
     */
    typedef struct
    {
        void* _impl;
    } rsid_device_controller;

    /* return new device controller pointer (or null on failure) */
    RSID_C_API rsid_device_controller* rsid_create_device_controller();

    /* destroy the authenticator and free its resources */
    RSID_C_API void rsid_destroy_device_controller(rsid_device_controller* device_controller);


    /* connect */
    RSID_C_API rsid_status rsid_connect_controller(rsid_device_controller* device_controller,
                                                   const rsid_serial_config* serial_config);

    /* disconnect */
    RSID_C_API void rsid_disconnect_controller(rsid_device_controller* device_controller);

    /* firmware version */
    RSID_C_API rsid_status rsid_query_firmware_version(rsid_device_controller* device_controller, char* output,
                                                       size_t output_length);

#ifdef __cplusplus
}
#endif
