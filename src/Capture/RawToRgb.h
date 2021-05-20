// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/Preview.h"

namespace RealSenseID
{
namespace Capture
{

void RotatedRaw2Rgb(const Image& src_img, Image& dst_img);

} // namespace Capture
} // namespace RealSenseID