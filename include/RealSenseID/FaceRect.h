// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseIDExports.h"
#include <cstdint>

#pragma pack(push)
#pragma pack(1)

namespace RealSenseID
{
/**
 * Detected face info
 * Upper left corner, width and height
 */
struct RSID_API FaceRect
{
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t w = 0;
    uint32_t h = 0;
};

} // namespace RealSenseID

#pragma pack(pop)
