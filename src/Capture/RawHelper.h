// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "StreamConverter.h"

namespace RealSenseID
{
namespace Capture
{
class RawHelper
{
    public:
        RawHelper(bool portrait) : _portrait(portrait) {};
        ~RawHelper();
        Image ConvertToRgb(const Image& src_img);
        Image RotateRaw(Image& src_img);

    private:
        void InitBuffer(int width, int height);
        Image _result_img;
        bool _portrait;
};


} // namespace Capture
} // namespace RealSenseID