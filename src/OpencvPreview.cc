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

#ifdef _WIN32
static const int PREVIEW_BACKEND = cv::CAP_MSMF;
#else 
static const int PREVIEW_BACKEND = cv::CAP_ANY;
#endif //_WIN32

namespace RealSenseID
{
cv::Mat ConvertW10ToBayerRG(const RealSenseID::Image& imageContainer)
{
    cv::Mat outImage = cv::Mat::zeros(imageContainer.height, imageContainer.width, CV_16U);

    int srcStep = 5;
    int dstStep = 4;

    int packSteps = imageContainer.width / dstStep;
    int packRectSteps = packSteps;

    uint8_t* __restrict srcPtr = (uint8_t*)imageContainer.buffer;
    uint16_t* __restrict dstPtr = (uint16_t*)outImage.data;

    int startPoint = 0;
    int step = (packSteps - packRectSteps) * srcStep;
    srcPtr += (0 * packSteps * srcStep) + startPoint;

    for (unsigned int rowIndex = 0; rowIndex < imageContainer.height; ++rowIndex)
    {
        for (int packIndex(0); packIndex < packRectSteps; ++packIndex)
        {
            uint16_t first8Bits = srcPtr[0] << 2;
            uint8_t extra = srcPtr[4];
            uint16_t last2Bits = (extra & 0x3);
            dstPtr[0] = first8Bits | last2Bits;

            first8Bits = srcPtr[1] << 2;
            last2Bits = (extra >> 2 & 0x3);
            dstPtr[1] = first8Bits | last2Bits;

            first8Bits = srcPtr[2] << 2;
            last2Bits = (extra >> (4) & 0x3);
            dstPtr[2] = first8Bits | last2Bits;

            first8Bits = srcPtr[3] << 2;
            last2Bits = (extra >> (6) & 0x3);
            dstPtr[3] = first8Bits | last2Bits;

            srcPtr += srcStep;
            dstPtr += dstStep;
        }
        srcPtr += step;
    }

    cv::cvtColor(outImage, outImage, cv::COLOR_BayerRG2BGR);
    outImage.convertTo(outImage, CV_8UC3, 256.0 / 1024);
    cv::rotate(outImage, outImage, cv::ROTATE_90_COUNTERCLOCKWISE);
    cv::resize(outImage, outImage, cv::Size(352, 640));
    cv::flip(outImage, outImage, 1);
    return outImage;
}

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
    CaptureResourceHandle(int camera_number, bool dumpModeOn)
    {
        // If camera_number is -1, we attempt to auto detect the device
        if (camera_number == -1)
        {
            constexpr int MaxCameraDevices = 10;
            constexpr double ExpectedWidth = 352.0f;
            constexpr double ExpectedHeight = 640.0f;
            constexpr double ExpectedBrightness = -1.0f;
            constexpr double ExpectedContrast = -1.0f;
            constexpr double ExpectedSaturation = -1.0f;
            constexpr double ExpectedTemperature = -1.0f;

            for (int i = 0; i < MaxCameraDevices; ++i)
            {
                auto capture_device = cv::VideoCapture(i, PREVIEW_BACKEND);

                auto width = capture_device.get(cv::CAP_PROP_FRAME_WIDTH);
                auto height = capture_device.get(cv::CAP_PROP_FRAME_HEIGHT);
                auto brightness = capture_device.get(cv::CAP_PROP_BRIGHTNESS);
                auto contrast = capture_device.get(cv::CAP_PROP_CONTRAST);
                auto saturation = capture_device.get(cv::CAP_PROP_SATURATION);
                auto temperature =  capture_device.get(cv::CAP_PROP_TEMPERATURE);

                if (width == ExpectedWidth && height == ExpectedHeight && ExpectedBrightness == brightness
                    && ExpectedContrast == contrast && ExpectedSaturation == saturation && ExpectedTemperature == temperature)
                {
                    LOG_DEBUG(LOG_TAG, "found preview device - index %d", i);
                    camera_number = i;
                    break;
                }
            }
            if (camera_number == -1)
            {
                LOG_ERROR(LOG_TAG, "failed to auto-detect. trying with camera number 0.");
                camera_number = 0;
            }
        }
        cap = cv::VideoCapture(camera_number, PREVIEW_BACKEND);
        cap.setExceptionMode(false);
        LOG_DEBUG(LOG_TAG, "Camera Streamer Backend is %s", cap.getBackendName().c_str());
        if (dumpModeOn) // requires custom fw support
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
            CaptureResourceHandle capture(_config.cameraNumber, _config.previewMode != PreviewMode::VGA);
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
                    container.faceRect.x = *(int*)(container.buffer + 7);
                    container.faceRect.y = *(int*)(container.buffer + 11);
                    container.faceRect.width = *(int*)(container.buffer + 15);
                    container.faceRect.height = *(int*)(container.buffer + 19);
                    
                    if (_config.previewMode == PreviewMode::FHD_Rect && container.size == 2592000)
                    {
                        container.width = 1920;
                        container.height = 1080;
                        auto rgbMat = ConvertW10ToBayerRG(container);
                        container.buffer = rgbMat.data;
                        container.size = (unsigned int)(rgbMat.step * rgbMat.rows);
                        container.width = 352;
                        container.height = 640;
                        container.stride = (unsigned int)(rgbMat.step);
                        container.faceRect.x = container.faceRect.x * container.width / 540;
                        container.faceRect.y = container.faceRect.y * container.height / 960;
                        container.faceRect.width = container.faceRect.width * container.width / 540;
                        container.faceRect.height = container.faceRect.height * container.height / 960;
                        if (container.faceRect.x > 0 &&
                            container.faceRect.y > 0 &&
                            container.faceRect.x + container.faceRect.width < container.width &&
                            container.faceRect.y + container.faceRect.height < container.height)
                            _callback->OnPreviewImageReady(container);
                        continue;
                    }
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
