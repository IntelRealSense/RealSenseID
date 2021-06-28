// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseIDExports.h"

namespace RealSenseID
{
class PreviewImpl;

/**
 * Preview modes
 */
enum class PreviewMode
{
    MJPEG_1080P = 0, // default
    MJPEG_720P = 1,
    RAW10_1080P = 2,
};

/**
 * Preview configuration
 */
struct RSID_API PreviewConfig
{
    int cameraNumber = -1; // attempt to auto detect by default
    PreviewMode previewMode = PreviewMode::MJPEG_1080P; // RAW10 requires custom fw support
    bool portraitMode = true;  // change Preview to get portrait or landscape images. Algo process is defined seperatly in DeviceConfig::CameraRotation
};

/**
 * RAW image metadata
 */
struct RSID_API ImageMetadata
{
    unsigned int timestamp = 0; // sensor timestamp (miliseconds)
    unsigned int status = 0;
    unsigned int sensor_id = 0;
    bool led = false;
    bool projector = false;
    bool is_snapshot = false;
};

/**
 * Image data for preview
 */
struct RSID_API Image
{
    unsigned char* buffer = nullptr;
    unsigned int size = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int stride = 0;
    unsigned int number = 0;
    ImageMetadata metadata;
};

/**
 * User defined callback for preview.
 * OnPreviewImageReady Callback will be used to provide preview image.
 * OnSnapshotImageReady Callback will be used to provide snapshot image -a cropped face image for any authentication/enrollment.
 * OnSnapshotImageReady relevant only if DeviceConfig::dump_mode == DumpMode::CroppedFace
 */
class RSID_API PreviewImageReadyCallback
{
public:
    virtual ~PreviewImageReadyCallback() = default;
    virtual void OnPreviewImageReady(const Image image) = 0;
    virtual void OnSnapshotImageReady(const Image image) {}; // Empty implemention for backward compability
};

/**
 * Preview Support. Use StartPreview to get callbacks for image frames
 */
class RSID_API Preview
{
public:
    explicit Preview(const PreviewConfig&);
    ~Preview();

    Preview(const Preview&) = delete;
    Preview& operator=(const Preview&) = delete;

    /**
     * Start preview.
     *
     * @param callback reference to callback object
     * @return True on success.
     */
    bool StartPreview(PreviewImageReadyCallback& callback);

    /**
     * Pause preview.
     *
     * @return True on success.
     */
    bool PausePreview();

    /**
     * Resume preview.
     *
     * @return True on success.
     */
    bool ResumePreview();

    /**
     * Stop preview.
     *
     * @return True on success.
     */
    bool StopPreview();

     /**
     * Convert Raw Image in_image to RGB24 and fill result in out_image
     * @param in_image an raw10 Image to convert
     * @param out_image an Image with pre-allocated buffer in size in_image.width * in_image.height * 3 
     * @return True on success.
     */
    bool RawToRgb(const Image& in_image, Image& out_image);

private:
    RealSenseID::PreviewImpl* _impl = nullptr;
};
} // namespace RealSenseID
