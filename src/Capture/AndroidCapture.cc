// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "AndroidCapture.h"
#include "Logger.h"
#include <stdexcept>
#include <android/api-level.h>
#include <unistd.h>
#include <sstream>

static const char* LOG_TAG = "AndroidCapture";
namespace RealSenseID
{
namespace Capture
{
static void ThrowIfFailed(const char *what, uvc_error_t uvc_err)
{
    if (uvc_err == UVC_SUCCESS)
        return;
    std::stringstream err_stream;
    err_stream << what << " failed with error: " << uvc_err << " - " << uvc_strerror(uvc_err);
    throw std::runtime_error(err_stream.str());
}

CaptureHandle::CaptureHandle(const PreviewConfig& config) : _config(config)
{
    int sys_dev = _config.cameraNumber;
    libusb_context *usb_context = NULL;
    auto api_level = android_get_device_api_level();
    // For some reason, in API level 23 (AOSP 6.0) uvc_init should be called with
    // NULL usb_context, while in later API levels we tested, weak authority was required
    // to be set.
    // This condition was added to fulfill the different requirements.
    // As more API levels will be tested this condition might change.
    if (api_level != 23) {
        int ret = libusb_set_option(usb_context, LIBUSB_OPTION_WEAK_AUTHORITY);
        if (0 < ret)
        {
            std::stringstream err_stream;
            err_stream << LOG_TAG << " - ERROR in libusb_set_option: " << ret;
            throw std::runtime_error(err_stream.str());
        }
    }
    uvc_error_t res;
    res = uvc_init(&ctx, usb_context);
    ThrowIfFailed("uvc_init", res);
    res = uvc_wrap(sys_dev, ctx, &devh);
    ThrowIfFailed("uvc_wrap", res);

     /* find stream by width, height. use default fps */
    res = uvc_get_stream_ctrl_format_size(devh, &ctrl, UVC_FRAME_FORMAT_ANY, VGA_WIDTH, VGA_HEIGHT, 0);
    ThrowIfFailed("uvc_get_stream_ctrl_format_size", res);

    res = uvc_stream_open_ctrl(devh, &stream, &ctrl);
    ThrowIfFailed("uvc_stream_open_ctrl", res);
    res = uvc_stream_start(stream, NULL, (void*)this, 0);
    ThrowIfFailed("uvc_stream_start", res);

    _stream_converter.InitStream(VGA_WIDTH,VGA_HEIGHT,_config.previewMode);
};

CaptureHandle::~CaptureHandle()
{
    uvc_stop_streaming(devh);
    LOG_DEBUG(LOG_TAG, "release camera");
    uvc_close(devh);
    uvc_exit(ctx);
}

bool CaptureHandle::Read(RealSenseID::Image* res)
{
    if (!stream){
        return false;
    }
    uvc_error_t err;
    err = uvc_stream_get_frame(stream, &frame, 10000);
    if (err != UVC_SUCCESS || frame == NULL)
    {
        return false;
    }

    return _stream_converter.Buffer2Image(res,(unsigned char*)frame->data,frame->data_bytes);
}
} // namespace Capture
} // namespace RealSenseID