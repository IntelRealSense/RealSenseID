// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "rsid_export.h"
#include "rsid_status.h"

#include <stdint.h>
#include <stddef.h>
#include "../../../../include/RealSenseID/FaceprintsDefines.h"

#define RSID_MAX_FACES 10 // max number of detected faces in single frame

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

    typedef struct
    {
        const char* port;
    } rsid_serial_config;

    typedef struct
    {
        rsid_camera_rotation_type camera_rotation;
        rsid_security_level_type security_level;
        rsid_algo_mode_type algo_mode;
        rsid_dump_mode dump_mode;
        rsid_matcher_confidence_level_type matcher_confidence_level;
        unsigned char max_spoofs;
        int gpio_auth_toggling;
    } rsid_device_config;

    typedef struct
    {
        /* upper left corner, width and height*/
        uint32_t x;
        uint32_t y;
        uint32_t w;
        uint32_t h;
    } rsid_face_rect;

    typedef struct
    {
        void* _impl;
    } rsid_authenticator;

// Typedefs here are based on those in FaceprintsDefines.h:
#ifdef __cplusplus
    typedef RealSenseID::DBFaceprintsElement rsid_faceprints_t;
#else
typedef struct DBFaceprintsElement rsid_faceprints_t;
#endif

#ifdef __cplusplus
    typedef RealSenseID::ExtractedFaceprintsElement rsid_extracted_faceprints_t;
#else
typedef struct ExtractedFaceprintsElement rsid_extracted_faceprints_t;
#endif

    typedef rsid_extracted_faceprints_t rsid_faceprints_match_element_t;

    typedef struct
    {
        char* user_id;
        rsid_faceprints_t faceprints;
    } rsid_user_faceprints_dble;

    /*
     * User defined callback to sign a given buffer before it is sent to the device.
     * Sign the buffer and copy the signature to the out_sig buffer (64 bytes)
     * Return 1 if succeeded and 0 otherwise.
     */
    typedef int (*rsid_sign_clbk)(const unsigned char* buffer, const unsigned int buffer_len, unsigned char* out_sig, void* ctx);

    /*
     * User defined callback to verify the buffer and the given signature.
     * Return 1 if succeeded and verified, 0 otherwise.
     */
    typedef int (*rsid_verify_clbk)(const unsigned char* buffer, const unsigned int buffer_len, const unsigned char* sig,
                                    const unsigned int siglen, void* ctx);

    typedef struct rsid_signature_clbk
    {
        rsid_sign_clbk sign_clbk;     /* user defined sign clbk */
        rsid_verify_clbk verify_clbk; /* user defined verify clbk */
        void* ctx;                    /* user defined context (optional, set to NULL if not needed) */
    } rsid_signature_clbk;

    /* rsid_authenticate() args */
    typedef void (*rsid_auth_status_clbk)(rsid_auth_status status, const char* user_id, void* ctx);
    typedef void (*rsid_auth_hint_clbk)(rsid_auth_status hint, void* ctx);
    typedef void (*rsid_face_detected_clbk)(const rsid_face_rect faces[], size_t n_faces, unsigned int ts, void* ctx);


    typedef struct rsid_auth_args
    {
        rsid_auth_status_clbk result_clbk;          /* result callback */
        rsid_auth_hint_clbk hint_clbk;              /* hint callback */
        rsid_face_detected_clbk face_detected_clbk; /* face detected callback (set to NULL if not needed)*/
        void* ctx;                                  /* user defined context (optional) */
    } rsid_auth_args;

    /* rsid_enroll() args */
    typedef void (*rsid_enroll_status_clbk)(rsid_enroll_status status, void* ctx);
    typedef void (*rsid_enroll_progress_clbk)(rsid_face_pose face_pose, void* ctx);
    typedef void (*rsid_enroll_hint_clbk)(rsid_enroll_status hint, void* ctx);
    typedef struct rsid_enroll_args
    {
        const char* user_id;                     /* user id. null terminated c string of ascii chars (max 30 chars + 1 terminating null) */
        rsid_enroll_status_clbk status_clbk;     /* status callback */
        rsid_enroll_progress_clbk progress_clbk; /* progress callback */
        rsid_enroll_hint_clbk hint_clbk;         /* hint calback */
        rsid_face_detected_clbk face_detected_clbk; /* face detected callback (set to NULL if not needed)*/
        void* ctx;                                  /* user defined context (optional, set to null if not needed) */
    } rsid_enroll_args;

    /* rsid_extract_faceprints_for_auth() args */
    typedef void (*rsid_faceprints_ext_status_clbk)(rsid_auth_status status, const rsid_extracted_faceprints_t* faceprints, void* ctx);
    typedef struct rsid_faceprints_ext_args // TODO: change name to rsid_auth_ext_args
    {
        rsid_faceprints_ext_status_clbk result_clbk; /* result callback */
        rsid_auth_hint_clbk hint_clbk;               /* hint callback */
        rsid_face_detected_clbk face_detected_clbk;  /* face detected callback (set to NULL if not needed)*/
        rsid_extracted_faceprints_t* faceprints;     /* extracted faceprints*/
        void* ctx;                                   /* user defined context (optional) */
    } rsid_faceprints_ext_args;

    typedef void (*rsid_enroll_ext_status_clbk)(rsid_enroll_status status, const rsid_faceprints_t* faceprints, void* ctx);

    typedef struct rsid_enroll_ext_args
    {
        rsid_enroll_ext_status_clbk status_clbk;    /* status callback */
        rsid_enroll_progress_clbk progress_clbk;    /* progress callback */
        rsid_enroll_hint_clbk hint_clbk;            /* hint callback */
        rsid_face_detected_clbk face_detected_clbk; /* face detected callback (set to NULL if not needed)*/
        void* ctx;                                  /* user defined context (optional, set to null if not needed) */
    } rsid_enroll_ext_args;

    /* rsid_match_faceprints() args */
    typedef struct rsid_match_args
    {
        rsid_faceprints_match_element_t new_faceprints;
        rsid_faceprints_t existing_faceprints;
        rsid_faceprints_t updated_faceprints;
        rsid_matcher_confidence_level_type matcher_confidence_level;
    } rsid_match_args;

    /* log callback */
    typedef void (*rsid_log_clbk)(rsid_log_level log_level, const char* msg);

    /* return new authenticator pointer (or null on failure) */
#ifdef RSID_SECURE
    RSID_C_API rsid_authenticator* rsid_create_authenticator(rsid_signature_clbk* signature_clbk);
#else
RSID_C_API rsid_authenticator* rsid_create_authenticator();
#endif //  RSID_SECURE


    /* destroy the authenticator and free its resources */
    RSID_C_API void rsid_destroy_authenticator(rsid_authenticator* authenticator);

    /* connect */
    RSID_C_API rsid_status rsid_connect(rsid_authenticator* authenticator, const rsid_serial_config* serial_config);

    /* disconnect */
    RSID_C_API void rsid_disconnect(rsid_authenticator* authenticator);

#ifdef RSID_SECURE
    /* send updated host ecdsa key to device, sign it with previous ecdsa key (at first pair can sign with dummy key)
     * output is device's ecdsa key
     */
    typedef struct rsid_pairing_args
    {
        char host_pubkey[64];          /* input host public key*/
        char host_pubkey_sig[64];      /* input host public key signature */
        char device_pubkey_result[64]; /* output device's public key*/
    } rsid_pairing_args;

    RSID_C_API rsid_status rsid_pair(rsid_authenticator* authenticator, rsid_pairing_args* pairing_args);

    /* unpair */
    RSID_C_API rsid_status rsid_unpair(rsid_authenticator* authenticator);
#endif // RSID_SECURE

    /* set authenticator settings to FW */
    RSID_C_API rsid_status rsid_set_device_config(rsid_authenticator* authenticator, const rsid_device_config* device_config);

    /* get authenticator settings from FW */
    RSID_C_API rsid_status rsid_query_device_config(rsid_authenticator* authenticator, rsid_device_config* device_config);

    /* enroll a user */
    RSID_C_API rsid_status rsid_enroll(rsid_authenticator* authenticator, const rsid_enroll_args* args);

    /* enroll a user with image
     * Note: The face should occupy at least 20% of image area
     */
    RSID_C_API rsid_enroll_status rsid_enroll_image(rsid_authenticator* authenticator, const char* user_id, const unsigned char* buffer,
                                                    unsigned width, unsigned height);

    /* enroll a user with image and return the faceprints*/
    RSID_C_API rsid_enroll_status rsid_extract_faceprints_from_image(rsid_authenticator* authenticator, const char* user_id,
                                                                     const unsigned char* buffer, unsigned width, unsigned height,
                                                                     rsid_faceprints_t* c_faceprints);

    /* authenticate a user */
    RSID_C_API rsid_status rsid_authenticate(rsid_authenticator* authenticator, const rsid_auth_args* args);

    /* authenticate in an infinite loop until rsid_cancel is called */
    RSID_C_API rsid_status rsid_authenticate_loop(rsid_authenticator* authenticator, const rsid_auth_args* args);

    /* detect spoof attempt */
    RSID_C_API rsid_status rsid_detect_spoof(rsid_authenticator* authenticator, const rsid_auth_args* args);

    /* authenticate in an infinite loop until cancel is called */
    RSID_C_API rsid_status rsid_cancel(rsid_authenticator* authenticator);

    /* remove given user the the device */
    RSID_C_API rsid_status rsid_remove_user(rsid_authenticator* authenticator, const char* user_id);

    /* remove all users from the device */
    RSID_C_API rsid_status rsid_remove_all_users(rsid_authenticator* authenticator);

    /* return library version */
    RSID_C_API const char* rsid_version();

    /* return compatible firmware version */
    RSID_C_API const char* rsid_compatible_firmware_version();

    RSID_C_API int rsid_is_fw_compatible_with_host(const char* fw_version);

    /* set log callback to be called when log with at least min_level is available */
    RSID_C_API void rsid_set_log_clbk(rsid_log_clbk clbk, rsid_log_level min_level, int do_formatting);


    /*
     * Query ids of all enrolled users from device.
     * On successfull operation, the result copied into the user_ids array and number_of_users is updated accordingly.
     * Note: char **user_ids should be a fully allocated array of user ids (each user id is char[31]).
     *       Use rsid_query_number_of_users(..) to find out how many slots to allocate in the array.
     */
    RSID_C_API rsid_status rsid_query_user_ids(rsid_authenticator* authenticator, char** user_ids, unsigned int* number_of_users);


    /*
     * Query ids of all enrolled users from device.
     * On successfull operation, the result copied into the result_buf and number_of_users is updated accordingly.
     * The result buf will contain all user ids (31 byte chunks).
     * Note: result_buf must be allocted with size of at least (number_of_users * 31)
     */

    RSID_C_API rsid_status rsid_query_user_ids_to_buf(rsid_authenticator* authenticator, char* result_buf, unsigned int* number_of_users);

    /*
     * Get number of enrolled users from device.
     * On successfull operation, the result is placed in number_of_users.
     */
    RSID_C_API rsid_status rsid_query_number_of_users(rsid_authenticator* authenticator, unsigned int* number_of_users);

    /*
     * Get the feature descriptors associated with the given userID
     * On successful operation, the descriptors are copied to faceprints.
     */
    RSID_C_API rsid_status rsid_get_users_faceprints(rsid_authenticator* authenticator, rsid_faceprints_t* user_features);

    /*
     * Insert (or update) all the users from the given array to the device's database.
     * On successful operation, each user's features are updated (if the user pre-existed), or the user is newly
     * enrolled,
     */
    RSID_C_API rsid_status rsid_set_users_faceprints(rsid_authenticator* authenticator, rsid_user_faceprints_dble* user_features,
                                                     const unsigned int number_of_users);

    /* Send device to standby */
    RSID_C_API rsid_status rsid_standby(rsid_authenticator* authenticator);

    /* Send device to hibernate (gpio wakeup) */
    RSID_C_API rsid_status rsid_hibernate(rsid_authenticator* authenticator);

    /* Unlock previously locked device due to too many spoof attempts*/
    RSID_C_API rsid_status rsid_unlock(rsid_authenticator* authenticator);

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
    RSID_C_API rsid_status rsid_connect_controller(rsid_device_controller* device_controller, const rsid_serial_config* serial_config);

    /* disconnect */
    RSID_C_API void rsid_disconnect_controller(rsid_device_controller* device_controller);

    /* firmware version */
    RSID_C_API rsid_status rsid_query_firmware_version(rsid_device_controller* device_controller, char* output, size_t output_length);

    /* serial number */
    RSID_C_API rsid_status rsid_query_serial_number(rsid_device_controller* device_controller, char* output, size_t output_length);

    /* send ping message and wait for valid ping reply */
    RSID_C_API rsid_status rsid_ping(rsid_device_controller* device_controller);

    /* query device log */
    RSID_C_API rsid_status rsid_fetch_log(rsid_device_controller* device_controller, char* output, size_t output_length);

    /* get color gains value from fw*/
    RSID_C_API rsid_status rsid_get_color_gains(rsid_device_controller* device_controller, int* red, int* blue);

    /* send color adjust message */
    RSID_C_API rsid_status rsid_set_color_gains(rsid_device_controller* device_controller, int red, int blue);

    /*******************************/
    /***** server mode methods *****/
    /*******************************/

    /* extract faceprints using enrollment flow */
    RSID_C_API rsid_status rsid_extract_faceprints_for_enroll(rsid_authenticator* authenticator, rsid_enroll_ext_args* args);

    /* extract faceprints using authentication flow */
    RSID_C_API rsid_status rsid_extract_faceprints_for_auth(rsid_authenticator* authenticator, rsid_faceprints_ext_args* args);

    /* extract faceprints in a loop using authentication flow */
    RSID_C_API rsid_status rsid_extract_faceprints_for_auth_loop(rsid_authenticator* authenticator, rsid_faceprints_ext_args* args);

    RSID_C_API rsid_match_result rsid_match_faceprints(rsid_authenticator* authenticator, rsid_match_args* args);


    RSID_C_API rsid_status rsid_set_license_key(const char* key);

    /*
     * get license key.
     * Note: result must point to an array of at least 37 chars before calling.
     */
    RSID_C_API rsid_status rsid_get_license_key(char result[37]);

    /* provide license to device*/
    RSID_C_API rsid_status rsid_provide_license(rsid_authenticator* authenticator);

    typedef void (*rsid_on_start_license_session)();
    typedef void (*rsid_on_end_license_session)(rsid_status status);

    /* enables automatic handling of license checks (enabled by default) using the cloud-based license server */
    RSID_C_API void rsid_enable_license_check_handler(
        rsid_authenticator* authenticator, rsid_on_start_license_session on_start_license_session /* set to NULL if callback not needed */,
        rsid_on_end_license_session on_end_license_session /* set to NULL if callback not needed */
    );

    /* disables automatic handling of license checks */
    RSID_C_API void rsid_disable_license_check_handler();

#ifdef __cplusplus
}
#endif //__cplusplus
