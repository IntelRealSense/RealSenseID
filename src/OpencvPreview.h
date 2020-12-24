// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/Preview.h"
#include <thread>
#include <atomic>
#include <string>

namespace cv
{
class Mat;
}

namespace RealSenseID
{
class OpencvPreview
{
public:
    explicit OpencvPreview(const PreviewConfig& config);
    ~OpencvPreview();

    void Start(PreviewImageReadyCallback& callback);
    void Stop();
    void Pause();
    void Resume();

private:
    PreviewConfig _config;
    std::thread _worker_thread;
    std::atomic_bool _canceled {false};
    std::atomic_bool _paused {false};
    PreviewImageReadyCallback* _callback = nullptr;

    void DumpFrame(const cv::Mat& frame, const std::string& folder);
};
} // namespace RealSenseID
