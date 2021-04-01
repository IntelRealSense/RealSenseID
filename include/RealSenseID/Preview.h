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
    VGA = 0,      // default
    FHD_Rect = 1, // result frame with face rect
    Dump = 2      // dump all frames
};

/**
 * Preview configuration
 */
struct RSID_API PreviewConfig
{
    int cameraNumber = -1; // attempt to auto detect by default
    PreviewMode previewMode = PreviewMode::VGA; // requires custom fw support
};

/**
 * Image data for preview
 */
struct RSID_API Image
{
    unsigned char* buffer;
    unsigned int size;
    unsigned int width;
    unsigned int height;
    unsigned int stride;
    unsigned int number;
    struct
    {
        unsigned int x;
        unsigned int y;
        unsigned int width;
        unsigned int height;
    } faceRect;
};

/**
 * User defined callback for preview.
 * Callback will be used to provide preview image.
 */
class RSID_API PreviewImageReadyCallback
{
public:
    virtual ~PreviewImageReadyCallback() = default;
    virtual void OnPreviewImageReady(const Image image) = 0;
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

private:
    RealSenseID::PreviewImpl* _impl = nullptr;
};
} // namespace RealSenseID
