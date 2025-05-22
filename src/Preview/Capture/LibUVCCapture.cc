// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2024 Intel Corporation. All Rights Reserved.

#include "LibUVCCapture.h"
#include "Logger.h"
#include <vector>
#include <cerrno>
#include <sstream>
#include <iomanip>
#include <memory>
#include <numeric>

#include "libuvc/libuvc.h"

// Enable for debugging. Noisy.
// #define RSID_DEBUG_UVC 1

namespace RealSenseID
{
namespace Capture
{

static const char* LOG_TAG = "LibUVCCapture";
static constexpr int MAX_CAM_INDEX = 100;

static std::vector<std::pair<std::string, std::string>> SupportedDevices()
{
    return {{"04d8", "00dd"}, {"2aad", "6373"}, {"414c", "6578"}};
}

static void ThrowIfFailed(const char* call, uvc_error_t status)
{
    if (status != UVC_SUCCESS)
    {
        std::stringstream err_stream;
        err_stream << call << "(...) failed with: " << uvc_strerror(status);
        throw std::runtime_error(err_stream.str().c_str());
    }
}

struct ContextWrapper
{
    uvc_context_t* ctx {nullptr};
    ContextWrapper() : ctx()
    {
        ThrowIfFailed("uvc_init", uvc_init(&ctx, nullptr));
    }
    ~ContextWrapper()
    {
        if (ctx)
            uvc_exit(ctx);
    }
};

class UVCStreamer
{
public:
    UVCStreamer(int camera_number, const StreamAttributes& stream_attributes);
    ~UVCStreamer();
    uvc_frame_t* Read() const;

private:
    StreamAttributes _stream_attributes;
    ContextWrapper _ctx_wrapper;
    void OpenDevice();
    void CheckDevice() const;
    uvc_device_handle_t* _dev_handle {nullptr};
    uvc_stream_handle_t* _stream_handle {nullptr};
    int _camera_index {0};
};

void UVCStreamer::OpenDevice()
{
    uvc_device_t** list {nullptr};
    uvc_device_t* dev {nullptr};
    try
    {
        ThrowIfFailed("uvc_get_device_list", uvc_get_device_list(_ctx_wrapper.ctx, &list));

        int idx = 0;
        while ((dev = list[idx]) != nullptr)
        {
            if (_camera_index == idx)
            {
                break;
            }
            idx++;
        }

        if (!dev)
        {
            throw std::runtime_error("Requested camera index is out of range.");
        }

        ThrowIfFailed("uvc_open", uvc_open(dev, &_dev_handle));
    }
    catch (std::exception&)
    {
        if (list)
        {
            uvc_free_device_list(list, 1);
        }
        throw;
    }
    if (list)
    {
        uvc_free_device_list(list, 1);
    }
}

void UVCStreamer::CheckDevice() const
{
    const auto dev = uvc_get_device(_dev_handle);
    uvc_device_descriptor_t* desc {nullptr};
    bool supported {false};
    std::exception_ptr ex_ptr {nullptr};

    try
    {
        ThrowIfFailed("uvc_get_device_descriptor", uvc_get_device_descriptor(dev, &desc));

        for (const auto& device : SupportedDevices())
        {
            const auto expected_vid = std::stoul(device.first, nullptr, 16);
            const auto expected_pid = std::stoul(device.second, nullptr, 16);
            if (desc->idVendor == expected_vid && desc->idProduct == expected_pid)
            {
                supported = true;
                break;
            }
        }
    }
    catch (const std::exception&)
    {
        ex_ptr = std::current_exception();
    }

    // Cleanup
    if (dev)
    {
        uvc_unref_device(dev);
    }
    if (desc)
    {
        uvc_free_device_descriptor(desc);
    }

    // Rethrow exception if caught
    if (ex_ptr)
    {
        std::rethrow_exception(ex_ptr);
    }

    if (!supported)
    {
        throw std::runtime_error("Device at index " + std::to_string(_camera_index) + " is *not* supported.");
    }
    else
    {
        LOG_DEBUG(LOG_TAG, "Device at index %d is recognized and supported.", _camera_index);
    }
}

UVCStreamer::UVCStreamer(int camera_number, const StreamAttributes& stream_attributes) :
    _stream_attributes(stream_attributes), _camera_index(camera_number)
{
    uvc_stream_ctrl_t ctrl;

    if (_camera_index < 0 || _camera_index > MAX_CAM_INDEX)
    {
        throw std::runtime_error("Requested camera index is out of range!");
    }

    OpenDevice();
#ifdef RSID_DEBUG_UVC
    uvc_print_diag(_dev_handle, stderr);
#endif
    CheckDevice();

    uvc_error_t status = UVC_ERROR_OTHER;
    if (_stream_attributes.format == MJPEG)
    {
        LOG_DEBUG(LOG_TAG, "Request capture format: MJPEG");
        std::vector<int> fps_range {30, 15, 14, 13}; // libuvc FPS range for MJPEG

        for (auto fps : fps_range)
        {
            status = uvc_get_stream_ctrl_format_size(_dev_handle, &ctrl, UVC_FRAME_FORMAT_MJPEG, static_cast<int>(_stream_attributes.width),
                                                     static_cast<int>(_stream_attributes.height), fps);
            if (status == UVC_SUCCESS)
            {
                LOG_INFO(LOG_TAG, "uvc_get_stream_ctrl_format_size: Found requested format at %d fps", fps);
                break;
            }
        }
    }
    else if (_stream_attributes.format == RAW)
    {
        LOG_DEBUG(LOG_TAG, "Request capture format: RAW/W10");
        std::vector<int> fps_range {6, 5, 4}; // libuvc FPS range for RAW/W10

        for (auto fps : fps_range)
        {
            // UVC_FRAME_FORMAT_W10 frame format was added to bundled libuvc-0.0.x-custom
            // Look for comments marked with `RSID_W10` in libuvc for changes made to libuvc
            status = uvc_get_stream_ctrl_format_size(_dev_handle, &ctrl, UVC_FRAME_FORMAT_W10, static_cast<int>(_stream_attributes.width),
                                                     static_cast<int>(_stream_attributes.height), fps);
            if (status == UVC_SUCCESS)
            {
                LOG_INFO(LOG_TAG, "uvc_get_stream_ctrl_format_size: Found requested format at %d fps", fps);
                break;
            }
        }
    }
    else
    {
        LOG_ERROR(LOG_TAG, "Request capture format: UNKNOWN");
        throw std::runtime_error("Unknown format requested!");
    }

    ThrowIfFailed("uvc_get_stream_ctrl_format_size", status);

#ifdef RSID_DEBUG_UVC
    uvc_print_stream_ctrl(&ctrl, stderr);
#endif

    ThrowIfFailed("uvc_stream_open_ctrl", uvc_stream_open_ctrl(_dev_handle, &_stream_handle, &ctrl));
    ThrowIfFailed("uvc_stream_start", uvc_stream_start(_stream_handle, nullptr, nullptr, 0));

    if (_stream_attributes.format == MJPEG)
    {
        Read(); // Read a single frame to clean-up first noisy frame.
    }
}

UVCStreamer::~UVCStreamer()
{
    if (_stream_handle)
    {
        uvc_stream_stop(_stream_handle);
    }
    if (_dev_handle)
    {
        uvc_close(_dev_handle);
    }
}

uvc_frame_t* UVCStreamer::Read() const
{
    uvc_frame_t* frame {nullptr};
    constexpr int32_t wait_usec {100000};

    uvc_error_t ret = uvc_stream_get_frame(_stream_handle, &frame, wait_usec);
    if (ret == UVC_SUCCESS && frame != nullptr)
    {
#if RSID_DEBUG_UVC
        LOG_DEBUG(LOG_TAG, "seq = %lu, frame_format = %d, width = %d, height = %d, length = %lu, metadata = %lu", frame->sequence,
                  frame->frame_format, frame->width, frame->height, frame->data_bytes, frame->metadata_bytes);
#endif

        return frame;
    }
    else if (ret != UVC_ERROR_TIMEOUT)
    {
        LOG_ERROR(LOG_TAG, "uvc_stream_get_frame: %s", uvc_strerror(ret));
        return nullptr;
    }

    return nullptr;
}

CaptureHandle::CaptureHandle(const PreviewConfig& config) : _config(config)
{
    _stream_converter = std::make_unique<StreamConverter>(_config);
    try
    {
        StreamAttributes attr = _stream_converter->GetStreamAttributes();
        _uvc_streamer = std::make_unique<UVCStreamer>(_config.cameraNumber, attr);
    }
    catch (const std::exception&)
    {
        _uvc_streamer.reset();
        throw;
    }
}

CaptureHandle ::~CaptureHandle()
{
    _uvc_streamer.reset();
    _stream_converter.reset();
}

bool CaptureHandle::Read(RealSenseID::Image* res) const
{
    bool valid_read {false};

    // read frame
    const auto frame = _uvc_streamer->Read();
    if (frame != nullptr)
    {
        buffer md_buffer;
        buffer frame_buffer;
        frame_buffer.data = static_cast<unsigned char*>(frame->data);
        frame_buffer.size = static_cast<unsigned int>(frame->data_bytes);
        frame_buffer.offset = 0;
        md_buffer.data = static_cast<unsigned char*>(frame->metadata);
        md_buffer.size = static_cast<unsigned int>(frame->metadata_bytes);
        md_buffer.offset = 0;

        if (frame_buffer.size == 0)
        {
            return false;
        }

        StreamAttributes attr = _stream_converter->GetStreamAttributes();
        if (attr.format == RAW && md_buffer.size == 0)
        {
            return false;
        }

        try
        {
            valid_read = _stream_converter->Buffer2Image(res, frame_buffer, md_buffer);
        }
        catch (...)
        {
            valid_read = false;
        }
    }

    return valid_read;
}
} // namespace Capture
} // namespace RealSenseID
