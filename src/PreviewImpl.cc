// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "PreviewImpl.h"
#include "Logger.h"
#include "RealSenseID/DiscoverDevices.h"
#include <chrono>
#include <stdexcept>

static const char* LOG_TAG = "Preview";

namespace RealSenseID
{
PreviewImpl::PreviewImpl(const PreviewConfig& config) : _config(config)
{
    if (config.cameraNumber == -1) // auto detection
    {
        std::vector<int> camera_numbers;
        try
        {
            camera_numbers = DiscoverCapture();
        }
        catch (const std::exception& ex)
        {
            throw std::runtime_error(std::string("UVC device detection failed:") + ex.what());
        }
        if (camera_numbers.size() == 0)
        {
            throw std::runtime_error(std::string("UVC device detection failed: No UVC devices found."));
        }
        _config.cameraNumber = camera_numbers[0];
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
            if (_config.previewMode == PreviewMode::RAW10_1080P)
                _raw_helper = std::make_unique<Capture::RawHelper>(_config.portraitMode);
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
                    if (_config.previewMode == PreviewMode::RAW10_1080P)
                    {
                        _callback->OnPreviewImageReady(_raw_helper->ConvertToRgb(container)); // sending RGB image to preview callback
                        if (container.metadata.is_snapshot)
                            _callback->OnSnapshotImageReady(_raw_helper->RotateRaw(container)); // sending raw image to snapshot callback   
                    }
                    else
                    {
                        if (container.metadata.is_snapshot)
                            _callback->OnSnapshotImageReady(container);
                        else
                            _callback->OnPreviewImageReady(container);
                    }
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

} // namespace RealSenseID
