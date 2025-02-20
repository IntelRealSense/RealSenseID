// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/RealSenseIDExports.h"
#include "RealSenseID/SerialConfig.h"

#include <vector>

namespace RealSenseID
{
struct RSID_API DeviceInfo
{
    static constexpr std::size_t MaxBufferSize = 256;

    char serialPort[MaxBufferSize];
};

std::vector<DeviceInfo> RSID_API DiscoverDevices();
std::vector<int> RSID_API DiscoverCapture();

} // namespace RealSenseID
