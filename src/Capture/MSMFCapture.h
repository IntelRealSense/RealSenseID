#pragma once
#include "RealSenseID/Preview.h"
#include "StreamConverter.h"

struct IMFSourceReader;
struct IMFMediaBuffer;


namespace RealSenseID
{
namespace Capture
{
class MsmfInitializer
{
public:
    MsmfInitializer();
    ~MsmfInitializer();
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
    MsmfInitializer _mf;
    IMFSourceReader* _video_src = nullptr;
    IMFMediaBuffer* _buf = nullptr;
    StreamConverter _stream_converter;
    PreviewConfig _config;
};
} // namespace Capture
} // namespace RealSenseID