// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseIDExports.h"

namespace RealSenseID
{
/**
 * Serial connection type
 */
enum class SerialType
{
    USB,
    UART
};

/**
 * Serial config of port and type.
 */
struct RSID_API SerialConfig
{
    SerialType serType = SerialType::USB;
    const char* port = nullptr;
};
} // namespace RealSenseID
