// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "AndroidPreview.h"
#include "Logger.h"
#include <unistd.h>
#include <chrono>
#include <thread>
#include <libusb.h>
#include <libuvc.h>

static const char* LOG_TAG = "AndroidPreview";

namespace RealSenseID
{

    class CaptureResourceHandle
    {
    public:
        CaptureResourceHandle(int sys_dev)
        {
            uvc_error_t res;
            res = uvc_init(&ctx, NULL);
            if (res != UVC_SUCCESS) {
                LOG_DEBUG(LOG_TAG,"uvc_init failed %s %d",uvc_strerror(res),res);
                return;
            }

            res = uvc_wrap(sys_dev,ctx,&devh);
            if (res != UVC_SUCCESS) {
                LOG_DEBUG(LOG_TAG, "uvc_find_device failed %s",uvc_strerror(res)); /* no devices found */
                return;
            }
            uvc_stream_ctrl_t ctrl;
            res = uvc_get_stream_ctrl_format_size(
                    devh, &ctrl, /* result stored in ctrl */
                    UVC_FRAME_FORMAT_ANY,
                    352, 640, 30 /* width, height, fps */
            );
            if (res != UVC_SUCCESS) {
                LOG_DEBUG(LOG_TAG, "uvc mode fail %s",uvc_strerror(res)); /* no devices found */
                return;
            }
            res = uvc_stream_open_ctrl(devh, &stream, &ctrl);
            if (res != UVC_SUCCESS) {
                LOG_DEBUG(LOG_TAG, "uvc_stream_open_ctrl  fail %s",uvc_strerror(res)); /* no devices found */
                return;
            }
            res = uvc_stream_start(stream, NULL, (void*) this, 0);
            if (res != UVC_SUCCESS) {
                LOG_DEBUG(LOG_TAG, "streaming  fail");
            }
        };

        ~CaptureResourceHandle()
        {
            LOG_DEBUG(LOG_TAG, "release camera");
        }

        bool read(uvc_frame_t **frame){
            uvc_error_t res;
            res = uvc_stream_get_frame(stream, frame,10000);
            if(res != UVC_SUCCESS || *frame == NULL)
            {
                return false;
            }
            return true;
        }


        void stop_stream(){
            uvc_stop_streaming(devh);
            uvc_close(devh);
            uvc_unref_device(dev);
            uvc_exit(ctx);
        }


    private:
        uvc_context_t *ctx =nullptr;
        uvc_device_t *dev = nullptr;
        uvc_device_handle_t *devh = nullptr;
        uvc_stream_handle_t *stream = nullptr;

    };

    AndroidPreview::AndroidPreview(const PreviewConfig& config):_config(config){};

    AndroidPreview::~AndroidPreview(){
        try
        {
            Stop();
        }
        catch (...)
        {
        }
    };

    void AndroidPreview::Start(PreviewImageReadyCallback& callback)
    {
        _callback = &callback;

        _paused = false;
        _canceled = false;

        if (_worker_thread.joinable())
        {
            return;
        }

        _worker_thread = std::thread([&]() {
            try{
                unsigned int frameNumber = 0;
                CaptureResourceHandle capture(_config.cameraNumber);
                LOG_DEBUG(LOG_TAG, "started!");
                while(!_canceled){
                    if (_paused)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds {100});
                        continue;
                    }
                    uvc_frame_t* frame = NULL;
                    bool res = capture.read(&frame);
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
                        unsigned char* data = (unsigned char*)frame->data;
                        container.buffer = data;
                        container.size = (unsigned int)(frame->data_bytes);
                        container.width = frame->width;
                        container.height = frame->height;
                        container.stride = (unsigned int)(frame->step);
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
    }

    void AndroidPreview::Stop()
    {
        LOG_DEBUG(LOG_TAG, "Stopping preview");
        _canceled = true;
        if (_worker_thread.joinable())
        {
            _worker_thread.join();
        }
        LOG_DEBUG(LOG_TAG, "Preview stopped");
    }

    void AndroidPreview::Pause()
    {
        LOG_DEBUG(LOG_TAG, "Pause preview");
        _paused = true;
    }

    void AndroidPreview::Resume()
    {
        LOG_DEBUG(LOG_TAG, "Resume preview");
        _paused = false;
    }
} // namespace rsid