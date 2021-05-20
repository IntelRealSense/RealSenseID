// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#include "rsid_export.h"
#include "rsid_status.h"

 
    typedef struct
    {
        void* _impl;
    } rsid_preview;

    typedef struct
    {
        int camera_number;
        rsid_preview_mode preview_mode;
    } rsid_preview_config;

    typedef struct
    {
        unsigned int x;
        unsigned int y;
        unsigned int width;
        unsigned int height;
    } rsid_face_rectangle;

    typedef struct
    {
        unsigned int timestamp;
        unsigned int status;
        unsigned int sensor_id;
        int led;
        int projector;
    }rsid_framedata;

    typedef struct
    {
        unsigned char* buffer;
        unsigned int size;
        unsigned int width;
        unsigned int height;
        unsigned int stride;
        unsigned int number;
        rsid_framedata metadata;
    } rsid_image;

    typedef void (*rsid_preview_clbk)(rsid_image image, void* ctx);

    /* return new device handle (or null on failure) */
    RSID_C_API rsid_preview* rsid_create_preview(const rsid_preview_config* preview_config);

    /* destroy the device handle and free its resources */
    RSID_C_API void rsid_destroy_preview(rsid_preview* preview_handle);

    /* start streaming of images. return 0 on error, 1 on sucess */
    RSID_C_API int rsid_start_preview(rsid_preview* preview_handle, rsid_preview_clbk clbk, void* ctx);

    /* pause streaming of images. return 0 on error, 1 on sucess */
    RSID_C_API int rsid_pause_preview(rsid_preview* preview_handle);

    /* resume streaming of images. return 0 on error, 1 on sucess */
    RSID_C_API int rsid_resume_preview(rsid_preview* preview_handle);

    /* stop streaming of images. return 0 on error, 1 on sucess */
    RSID_C_API int rsid_stop_preview(rsid_preview* preview_handle);

    /* convert raw to rgb. return 0 on error, 1 on sucess */
    RSID_C_API int rsid_raw_to_rgb(rsid_preview* preview_handle,const rsid_image* in, rsid_image* out);

#ifdef __cplusplus
}
#endif //__cplusplus
