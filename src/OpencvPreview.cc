// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "OpencvPreview.h"
#include "Logger.h"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <chrono>
#include <thread>

static const char* LOG_TAG = "OpencvPreview";

namespace RealSenseID
{
OpencvPreview::OpencvPreview(const PreviewConfig& config) : _config(config)
{
}

OpencvPreview::~OpencvPreview()
{
    try
    {
        Stop();
    }
    catch (...)
    {
    }
}

class CaptureResourceHandle
{
    static const unsigned int RAW_FRAME_WIDTH = 1920;
    static const unsigned int RAW_FRAME_HEIGHT = 1080;

public:
    CaptureResourceHandle(int camera_number, bool debug_mode)
    {
#ifdef WIN32
        cap = cv::VideoCapture(camera_number, cv::CAP_MSMF);
#else
        cap = cv::VideoCapture(camera_number, 0);
#endif
        cap.setExceptionMode(false);
        LOG_DEBUG(LOG_TAG, "Camera Streamer Backend is %s", cap.getBackendName().c_str());
        if (debug_mode) // requires custom fw support
        {
            cap.set(cv::CAP_PROP_MODE, 0);
            cap.set(cv::CAP_PROP_CONVERT_RGB, 0);
            cap.set(cv::CAP_PROP_FRAME_WIDTH, RAW_FRAME_WIDTH);
            cap.set(cv::CAP_PROP_FRAME_HEIGHT, RAW_FRAME_HEIGHT);
        }
    };
    ~CaptureResourceHandle()
    {
        LOG_DEBUG(LOG_TAG, "release camera");
        cap.release();
    }
    cv::VideoCapture* operator->()
    {
        return &cap;
    }

private:
    cv::VideoCapture cap;
};


void OpencvPreview::Start(PreviewImageReadyCallback& callback)
{
    LOG_DEBUG(LOG_TAG, "Preview for camera %d start", _config.cameraNumber);

    _callback = &callback;
    _paused = false;
    _canceled = false;

    // If there is already running thread do nothing
    if (_worker_thread.joinable())
    {
        return;
    }

    _worker_thread = std::thread([&]() {
        try
        {
            unsigned int frameNumber = 0;
            CaptureResourceHandle capture(_config.cameraNumber, _config.debugMode);
            cv::Mat frame;
            while (!_canceled)
            {
                if (_paused)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds {100});
                    continue;
                }
                bool res = capture->read(frame);
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
                    RealSenseID::Image container;
                    unsigned char* data = (unsigned char*)frame.data;
                    container.buffer = data;
                    container.size = (unsigned int)(frame.step[0] * frame.rows);
                    container.width = frame.cols;
                    container.height = frame.rows;
                    container.stride = (unsigned int)(frame.step);
                    container.number = frameNumber++;
                    _callback->OnPreviewImageReady(container);
                }
                else
                {
                    LOG_ERROR(LOG_TAG, "Reading frame failed");
                    _canceled = true;
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
}

void OpencvPreview::Stop()
{
    LOG_DEBUG(LOG_TAG, "Stopping preview");
    _canceled = true;
    if (_worker_thread.joinable())
    {
        _worker_thread.join();
    }
    LOG_DEBUG(LOG_TAG, "Preview stopped");
}

void OpencvPreview::Pause()
{
    LOG_DEBUG(LOG_TAG, "Pause preview");
    _paused = true;
}

void OpencvPreview::Resume()
{
    LOG_DEBUG(LOG_TAG, "Resume preview");
    _paused = false;
}

} // namespace RealSenseID
