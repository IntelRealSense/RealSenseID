#pragma once

#include "RealSenseID/Preview.h"
#include "StreamConverter.h"
#include <vector>

namespace RealSenseID
{
namespace Capture
{
struct buffer{
    unsigned char* data = nullptr;
    unsigned int size = 0;
};

class CaptureHandle
{
public:
    explicit CaptureHandle(const PreviewConfig& config);
    ~CaptureHandle();
    bool Read(RealSenseID::Image* res);

    // prevent copy or assignment
    // only single connection is allowed to a capture device.
    CaptureHandle(const CaptureHandle&) = delete;
    void operator=(const CaptureHandle&) = delete;

private:
    int _fd = 0;
    std::vector<buffer> _buffers;
    StreamConverter _stream_converter;
    PreviewConfig _config;
};
} // namespace Capture
} // namespace RealSenseID
