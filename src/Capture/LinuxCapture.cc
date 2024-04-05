#include "LinuxCapture.h"
#include "Logger.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sstream>
#include <cstring>
#include <iostream>
#include <memory>
#include <linux/videodev2.h>

#ifdef V4L2_META_FMT_UVC
constexpr bool METADATA_AVAILABLE = true;
#else
constexpr bool METADATA_AVAILABLE = false;
#pragma message ( "\n V4L2_META_FMT_UVC was not defined. No metadata avaiable")
#endif

constexpr auto LOCAL_V4L2_BUF_TYPE_META_CAPTURE = (v4l2_buf_type)(13);

namespace RealSenseID
{
namespace Capture
{
static const char* LOG_TAG = "LinuxCapture";

static const std::string VIDEO_DEV = "/dev/video";
constexpr int FAILED_V4L = -1;
constexpr unsigned int MD_OFFSET = 22;

static void ThrowIfFailed(const char* what, int res)
{
    if (res != FAILED_V4L)
        return;
    std::stringstream err_stream;
    err_stream << what << " v4l failed with error.";
    throw std::runtime_error(err_stream.str());
}

void CleanMMAPBuffers(std::vector<buffer>& buffer_list)
{
    int ret;
    for (size_t i = 0; i < buffer_list.size(); i++)
    {
        if (buffer_list[i].size > 0)
        {
            ret = munmap(buffer_list[i].data, buffer_list[i].size); // unmap buffers
            if (ret == FAILED_V4L)
                LOG_ERROR(LOG_TAG, " unmapping buffer %zu failed", i);
        }
    }
}

class V4lNode
{
public:
    V4lNode(int camera_number,v4l2_format format);
    ~V4lNode();
    buffer Read();

private:
    int _fd = 0;
    unsigned int _type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    std::vector<buffer> _buffers;
};

V4lNode::V4lNode(int camera_number,v4l2_format format):_type(format.type)
{
    try
    {
        // open device
        std::string dev_md = VIDEO_DEV + std::to_string(camera_number);
        _fd = open(dev_md.c_str(), O_RDWR | O_NONBLOCK, 0);
        ThrowIfFailed("fd_md", _fd);

        // set format
        ThrowIfFailed("set  stream",ioctl(_fd, VIDIOC_S_FMT, &format));

        // request buffers
        v4l2_requestbuffers req;
        std::memset(&req, 0, sizeof(req));
        req.count = 4;
        req.type = _type;
        req.memory = V4L2_MEMORY_MMAP;
        ThrowIfFailed("req buffer", ioctl(_fd, VIDIOC_REQBUFS, &req));

        // query buffers
        _buffers = std::vector<buffer>(req.count);
        for (size_t i = 0; i < _buffers.size(); i++)
        {
            v4l2_buffer buf;
            std::memset(&buf, 0, sizeof(buf));
            buf.type = _type;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = static_cast<uint32_t>(i);
            ThrowIfFailed("Query buffers", ioctl(_fd, VIDIOC_QUERYBUF, &buf));
            _buffers[i].data = static_cast<unsigned char*>(
                mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, buf.m.offset));
            ThrowIfFailed("mmap", (_buffers[i].data == MAP_FAILED) - 2);
            _buffers[i].size = buf.length;
        }

        // start stream
        v4l2_buf_type stream_type = (v4l2_buf_type)_type;
        ThrowIfFailed("start stream", ioctl(_fd, VIDIOC_STREAMON, &stream_type));

        // queue buffers
        for (unsigned int i = 0; i < req.count; i++)
        {
            v4l2_buffer buf;
            std::memset(&buf, 0, sizeof(buf));
            buf.type = _type;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            ThrowIfFailed("req buffer", ioctl(_fd, VIDIOC_QBUF, &buf));
        }
    }
    catch(const std::exception& e)
    {
        if(_fd)
            close(_fd);
        CleanMMAPBuffers(_buffers);
        throw e;
    }
}

V4lNode::~V4lNode()
{
    ioctl(_fd, VIDIOC_STREAMOFF, _type); // shutdown stream
    if(_fd)
        close(_fd);
    CleanMMAPBuffers(_buffers);
}

buffer V4lNode::Read()
{
    buffer res;
    struct v4l2_buffer buf;
    std::memset(&buf, 0, sizeof(buf));

    struct timeval tv; 
    std::memset(&tv, 0, sizeof(tv));
    tv.tv_sec = 0; // max time to wait for next frame
    tv.tv_usec = 100;

    buf.type = _type;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(_fd, &fds);
    ThrowIfFailed("wait for frame", select(_fd, &fds, NULL, NULL, &tv));
    if (ioctl(_fd, VIDIOC_DQBUF, &buf) == FAILED_V4L){ // dequeue frame from buffer. 
        return res; //returns zero-length buffer
    }

    res = _buffers[buf.index];
    res.size = buf.bytesused;
    
    buf.type = _type; 
    buf.memory = V4L2_MEMORY_MMAP;
    ThrowIfFailed("qbuffer", ioctl(_fd, VIDIOC_QBUF, &buf)); // queue next frame

    return res;
}

CaptureHandle::CaptureHandle(const PreviewConfig& config): _config(config)
{
    _stream_converter = std::make_unique<StreamConverter>(_config); 

    if(METADATA_AVAILABLE)
    {
        // set metadata node
        try
        {
            v4l2_format md_format;
            std::memset(&md_format, 0, sizeof(md_format));

            md_format.type = LOCAL_V4L2_BUF_TYPE_META_CAPTURE;

            // Assume for each streaming node with index N there is a metadata node with index (N+1)
            _md_node = std::make_unique<V4lNode>(_config.cameraNumber + 1 ,md_format);
        }
        catch(const std::exception& ex)
        {
            LOG_ERROR(LOG_TAG,"failed to create metadata conntection. %s.",ex.what());
            _md_node.reset();
        }
    }

    // set video node
    try
    {
        v4l2_format format;
        std::memset(&format, 0, sizeof(format));

        StreamAttributes attr = _stream_converter->GetStreamAttributes();
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(attr.format == MJPEG)
            format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
        format.fmt.pix.width = attr.width;
        format.fmt.pix.height = attr.height;      
        _frame_node = std::make_unique<V4lNode>(_config.cameraNumber,format);
    }
    catch (const std::exception& ex)
    {
        _frame_node.reset();
        _md_node.reset();
        throw ex;
    }
}

CaptureHandle ::~CaptureHandle()
{
    _frame_node.reset();
    _md_node.reset();
}

bool CaptureHandle::Read(RealSenseID::Image* res)
{
    buffer frame_buffer;
    buffer md_buffer;
    bool valid_read;

    // read frame
    frame_buffer = _frame_node->Read();
    if(frame_buffer.size == 0){
        return false;
    }

    // read metadata
    if(_md_node)
    {
        md_buffer = _md_node->Read();
        md_buffer.offset = MD_OFFSET;
    }

    valid_read = _stream_converter->Buffer2Image(res, frame_buffer,md_buffer);

    return valid_read;
}
} // namespace Capture
} // namespace RealSenseI