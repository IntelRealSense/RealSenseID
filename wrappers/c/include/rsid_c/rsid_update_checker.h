// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2024 Intel Corporation. All Rights Reserved.

#pragma once

#include "rsid_export.h"
#include "rsid_status.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#include "rsid_export.h"
#include "rsid_client.h"
    //typedef struct
    //{
    //    void* _impl;
    //} rsid_update_checker;

    /* firmware update related settings */
    typedef struct
    {
        uint64_t sw_version;
        uint64_t fw_version;
        const char* sw_version_str;
        const char* fw_version_str;
        const char* release_url;
        const char* release_notes_url;
    } rsid_release_info;

       
    /* return new updater checker handle (or null on failure) */
    /*RSID_C_API rsid_update_checker* rsid_create_update_updater();*/

    /* 
     * get release info from remote server 
     * return 0 on error, 1 on sucess 
     * Note: You must call rsid_free_release_info to free the result members after use. 
     * It doesn't free the struct itself
     */
    RSID_C_API rsid_status rsid_get_remote_release_info(rsid_release_info* result);

    /* 
     * get local release info from device and library
     *  return 0 on error, 1 on sucess 
     * Note: You must call rsid_free_release_info to free the result members after use
     */   
    RSID_C_API rsid_status rsid_get_local_release_info(const rsid_serial_config* serial_config,
                                                       rsid_release_info* result);


    /* 
     * Free rsid_release_info struct members. 
     * Note: It doesn't free the struct itself
     */
    RSID_C_API void rsid_free_release_info(rsid_release_info* release_info);

#ifdef __cplusplus
}
#endif //__cplusplus
