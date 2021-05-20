// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "PreviewImpl.h"
#include "Logger.h"
#include "RealSenseID/DiscoverDevices.h"
#include "RawToRgb.h"
#include <chrono>

static const char* LOG_TAG = "Preview";

namespace RealSenseID
{
PreviewImpl::PreviewImpl(const PreviewConfig& config) : _config(config)
{
    if (config.cameraNumber == -1)
    {
        std::vector<int> camera_numbers;
        try
        {
            camera_numbers = DiscoverCapture();
        }
        catch (...)
        {
        }
        _config.cameraNumber = (camera_numbers.size() > 0) ? camera_numbers[0] : 0;
    }
};

PreviewImpl::~PreviewImpl()
{
    try
    {
        StopPreview();
    }
    catch (...)
    {
    }
}

bool PreviewImpl::StartPreview(PreviewImageReadyCallback& callback)
{
    _callback = &callback;

    _paused = false;
    _canceled = false;

    if (_worker_thread.joinable())
    {
        return false;
    }

    _worker_thread = std::thread([&]() {
        try
        {
            _capture = std::make_unique<Capture::CaptureHandle>(_config);
            unsigned int frameNumber = 0;
            LOG_DEBUG(LOG_TAG, "Preview started!");
            while (!_canceled)
            {
                if (_paused)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds {100});
                    continue;
                }
                RealSenseID::Image container;
                bool res = _capture->Read(&container);
                if (_canceled)
                {
                    break;
                }
                if (_paused)
                {
                    continue;
                }
                if (res)
                {
                    container.number = frameNumber++;
                    _callback->OnPreviewImageReady(container);
                }
                else
                {
                    continue;
                }
            }
        }
        catch (const std::exception& ex)
        {
            LOG_ERROR(LOG_TAG, "Streaming ERROR : %s", ex.what());
            _canceled = true;
        }
        catch (...)
        {
            LOG_ERROR(LOG_TAG, "Streaming unknonwn exception");
            _canceled = true;
        }
        _capture.reset();
    });
    return true;
}

bool PreviewImpl::PausePreview()
{
    _paused = true;
    return true;
}

bool PreviewImpl::ResumePreview()
{
    _paused = false;
    return true;
}

bool PreviewImpl::StopPreview()
{
    _canceled = true;
    if (_worker_thread.joinable())
    {
        _worker_thread.join();
    }
    return true;
}

bool PreviewImpl::RawToRgb(const Image& in_image, Image& out_image)
{
    if (in_image.buffer == nullptr || out_image.buffer == nullptr || in_image.size == 0)
        return false;
    if (out_image.size != in_image.width * in_image.height * 3) // check for valid buffer of out_image
    {
        LOG_DEBUG(LOG_TAG, "RawToRgb out_image is in size %d.need to be in_image width*height*3 =%d", out_image.size,
                  in_image.width * in_image.height * 3);
        return false;
    }
    if (((in_image.width * in_image.height / 4) * 5) != in_image.size) // check for valid w10 image 10bpp
    {
        LOG_DEBUG(LOG_TAG, "RawToRgb in_image is not a valid raw10 image");
        return false;
    }
    RotatedRaw2Rgb(in_image, out_image);
}
} // namespace RealSenseID
