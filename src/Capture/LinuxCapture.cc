#include "LinuxCapture.h"
#include "Logger.h"
#include <linux/videodev2.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sstream>
#include <cstring>
#include <iostream>
#include <memory>

namespace RealSenseID
{
namespace Capture
{
static const char* LOG_TAG = "LinuxCapture";

static const std::string VIDEO_DEV = "/dev/video";
static const int FAILED_V4L = -1;

static void ThrowIfFailed(const char* what, int res)
{
    if (res != FAILED_V4L)
        return;
    std::stringstream err_stream;
    err_stream << what << " v4l failed with error.";
    throw std::runtime_error(err_stream.str());
}

v4l2_format GetDefaultFormat(bool is_debug)
{ 
    v4l2_format format = {0};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (is_debug)
    {
        format.fmt.pix.width = RAW_WIDTH;
        format.fmt.pix.height = RAW_HEIGHT;
    }
    else
    {
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        format.fmt.pix.width = VGA_WIDTH;
        format.fmt.pix.height = VGA_HEIGHT;
    }
    return format;
}

void CleanMMAPBuffers(std::vector<buffer>& buffer_list)
{
    int ret;
    for (int i = 0; i < buffer_list.size(); i++)
    {
        if (buffer_list[i].size > 0)
        {
            ret = munmap(buffer_list[i].data, buffer_list[i].size); // unmap buffers
            if (ret == FAILED_V4L)
                LOG_ERROR(LOG_TAG, " unmapping buffer %d failed", i);
        }
    }
}

CaptureHandle::CaptureHandle(const PreviewConfig& config): _config(config)
{
    v4l2_format format;
    std::string dev = VIDEO_DEV + std::to_string(_config.cameraNumber);
    _fd = open(dev.c_str(), O_RDWR | O_NONBLOCK, 0);
    ThrowIfFailed("fd", _fd);

    try
    {
        // set format
        format = GetDefaultFormat(_config.previewMode != PreviewMode::VGA);
        ThrowIfFailed("set format", ioctl(_fd, VIDIOC_S_FMT, &format));

        // set memory mode
        v4l2_requestbuffers req = {0};
        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        ThrowIfFailed("set memory mode", ioctl(_fd, VIDIOC_REQBUFS, &req));
        LOG_DEBUG(LOG_TAG, " got %d buffers", req.count);

        // create buffers
        _buffers = std::vector<buffer>(req.count);
        for (int i = 0; i < _buffers.size(); i++)
        {
            v4l2_buffer buf = {0};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            ThrowIfFailed("req buffer", ioctl(_fd, VIDIOC_QUERYBUF, &buf));
            _buffers[i].data = static_cast<unsigned char*>(
                mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, buf.m.offset));
            ThrowIfFailed("mmap", (_buffers[i].data == MAP_FAILED) - 2);
            _buffers[i].size = buf.length;
        }

        // start stream
        unsigned int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ThrowIfFailed("start stream", ioctl(_fd, VIDIOC_STREAMON, &type));

        // create buffers
        for (int i = 0; i < req.count; i++)
        {
            v4l2_buffer buf = {0};
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            ThrowIfFailed("req buffer", ioctl(_fd, VIDIOC_QBUF, &buf));
        }
    }
    catch (const std::exception& ex)
    {
        CleanMMAPBuffers(_buffers);
        if (_fd)
            close(_fd);
        throw ex;
    }
    // set stream attr and init buffer
    _stream_converter.InitStream(format.fmt.pix.width, format.fmt.pix.height, _config.previewMode);
}

CaptureHandle ::~CaptureHandle()
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(_fd, VIDIOC_STREAMOFF, &type); // shutdown stream
    CleanMMAPBuffers(_buffers);
    if (_fd)
        close(_fd);
}

bool CaptureHandle::Read(RealSenseID::Image* res)
{
    bool valid_read = false;
    struct v4l2_buffer buf = {0};
    struct timeval tv = {0}; 
    tv.tv_sec = 1; // max time to wait for next frame

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(_fd, &fds);
    ThrowIfFailed("wait for frame", select(_fd + 1, &fds, NULL, NULL, &tv)); //wait for frame 
    if (ioctl(_fd, VIDIOC_DQBUF, &buf) == FAILED_V4L){ // dequeue frame from buffer. 
        return false;
    }

    //now buf.index is the index of the latest buffer filled

    valid_read = _stream_converter.Buffer2Image(res, _buffers[buf.index].data, _buffers[buf.index].size);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
    buf.memory = V4L2_MEMORY_MMAP;
    ThrowIfFailed("qbuffer", ioctl(_fd, VIDIOC_QBUF, &buf)); // queue next frame

    return valid_read;
}
} // namespace Capture
} // namespace RealSenseI