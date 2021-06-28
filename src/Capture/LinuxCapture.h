#pragma once

#include "RealSenseID/Preview.h"
#include "StreamConverter.h"
#include <vector>
#include <memory>

namespace RealSenseID
{
namespace Capture
{

class V4lNode;

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
    std::unique_ptr<StreamConverter> _stream_converter;
    std::unique_ptr<V4lNode> _frame_node;
    std::unique_ptr<V4lNode> _md_node;
    PreviewConfig _config;
};
} // namespace Capture
} // namespace RealSenseID
