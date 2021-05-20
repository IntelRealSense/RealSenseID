// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/Preview.h"

#include <thread>
#include <atomic>

#ifdef ANDROID
#include "AndroidCapture.h"
#elif LINUX
#include "LinuxCapture.h"
#elif _WIN32
#include "MSMFCapture.h"
#endif

namespace RealSenseID
{
class PreviewImpl
{
public:
    ~PreviewImpl();
    explicit PreviewImpl(const PreviewConfig& config);
    bool StartPreview(PreviewImageReadyCallback& callback);
    bool PausePreview();
    bool ResumePreview();
    bool StopPreview();
    bool RawToRgb(const Image& in_image,Image& out_image);

private:
    PreviewConfig _config;
    std::thread _worker_thread;
    std::atomic_bool _canceled {false};
    std::atomic_bool _paused {false};
    PreviewImageReadyCallback* _callback = nullptr;
    std::unique_ptr<Capture::CaptureHandle> _capture;
};
} // namespace RealSenseID
