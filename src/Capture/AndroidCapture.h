// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once
#include "RealSenseID/Preview.h"
#include "StreamConverter.h"
#include <libusb.h>
#include <libuvc.h>


namespace RealSenseID
{
namespace Capture
{
class CaptureHandle
{
public:
    explicit CaptureHandle(const PreviewConfig& config);
    ~CaptureHandle();
    bool Read(RealSenseID::Image* container);

    // prevent copy or assignment
    // only single connection is allowed to a captre device.
    CaptureHandle(const CaptureHandle&) = delete;
    void operator=(const CaptureHandle&) = delete;

private:
    uvc_context_t* ctx = nullptr;
    uvc_device_t* dev = nullptr;
    uvc_device_handle_t* devh = nullptr;
    uvc_stream_handle_t* stream = nullptr;
    uvc_stream_ctrl_t ctrl;
    uvc_frame_t* frame = nullptr;
    StreamConverter _stream_converter;
    PreviewConfig _config;
};
} // namespace Capture
} // namespace RealSenseID