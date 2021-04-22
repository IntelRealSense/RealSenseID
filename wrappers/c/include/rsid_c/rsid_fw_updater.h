// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "rsid_export.h"
#include "rsid_status.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#include "rsid_export.h"
    typedef struct
    {
        void* _impl;
    } rsid_fw_updater;

    /* firmware update related settings */
    typedef struct
    {
        const char* port; // serial port to perform the update on
        int force_full;   // force full update of all modules
    } rsid_fw_update_settings;

    /*
     * User defined callback to handle firmware update progress.
     * Receives the progress as a float in the range of 0.0f-1.0f.
     */
    typedef void (*rsid_progress_callback)(float progress);

    typedef struct
    {
        rsid_progress_callback progress_callback; /* user defined progress callback */
    } rsid_fw_update_event_handler;

    /* return new fw updater handle (or null on failure) */
    RSID_C_API rsid_fw_updater* rsid_create_fw_updater();

    /* destroy the fw updater handle and free its resources */
    RSID_C_API void rsid_destroy_fw_updater(rsid_fw_updater* handle);

    /* check firmware compatibility with host */
    RSID_C_API int rsid_is_compatible_with_host(rsid_fw_updater* handle, const char* fw_version);

    /* extract version from firmware binary package */
    RSID_C_API int rsid_extract_firmware_version(rsid_fw_updater* handle, const char* bin_path, char* new_fw_version,
                                                 size_t new_fw_version_length, char* new_recognition_version,
                                                 size_t new_recognition_version_size);

    /* performs a firmware update */
    RSID_C_API rsid_status rsid_update_firmware(rsid_fw_updater* handle,
                                                const rsid_fw_update_event_handler* event_handler,
                                                rsid_fw_update_settings settings, const char* bin_path,
                                                int exclude_recognition);

#ifdef __cplusplus
}
#endif //__cplusplus
