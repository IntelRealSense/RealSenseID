// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "PreviewImpl.h"
#include "Logger.h"
#include "RealSenseID/DiscoverDevices.h"
#include <chrono>

#ifdef ANDROID
#include "AndroidCapture.h"
#elif LINUX
#include "LinuxCapture.h"
#elif _WIN32
#include "MSMFCapture.h"
#endif

static const char* LOG_TAG = "Preview";

namespace RealSenseID
{
PreviewImpl::PreviewImpl(const PreviewConfig& config) : _config(config)
{
    if(config.cameraNumber == -1) {
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
            unsigned int frameNumber = 0;
            Capture::CaptureHandle capture(_config);
            LOG_DEBUG(LOG_TAG, "Preview started!");
            while (!_canceled)
            {
                if (_paused) {
                    std::this_thread::sleep_for(std::chrono::milliseconds{100});
                    continue;
                }
                RealSenseID::Image container;
                bool res = capture.Read(&container);
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
