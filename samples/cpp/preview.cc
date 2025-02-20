// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/Preview.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <cstdio>

class PreviewRender : public RealSenseID::PreviewImageReadyCallback
{
public:
    void OnPreviewImageReady(const RealSenseID::Image image)
    {
        std::cout << "frame #" << image.number << ": " << image.width << "x" << image.height << " (" << image.size << " bytes)"
                  << std::endl;
    }
};


int main()
{
    RealSenseID::PreviewConfig p_conf; // PreviewConfig default attributes are cameraNumber=-1 (auto detection) and previewMode=MJPEG_1080
    RealSenseID::Preview preview(p_conf);
    PreviewRender image_clbk;

    bool success = preview.StartPreview(image_clbk);
    std::cout << "run preview for 30 sec" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(30));
    success = preview.StopPreview();
}
