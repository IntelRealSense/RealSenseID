// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2024 Intel Corporation. All Rights Reserved.

#include "JPEGDecoder.h"
#include "RealSenseID/Preview.h"
#pragma once

namespace RealSenseID
{
namespace Capture
{

class JPEGWICDecoder
{
public:
    JPEGWICDecoder();
    ~JPEGWICDecoder();
    bool DecodeJpeg(Image* res, buffer frame_buffer, size_t max_height, size_t max_width) const;

private:
    static void InitDecompressor();
};

} // namespace Capture
} // namespace RealSenseID