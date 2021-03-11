// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseIDExports.h"

namespace RealSenseID
{
/**
 * Android Serial config to hold file descriptor and endpoints found by application layer.
 */
struct RSID_API AndroidSerialConfig
{
    int fileDescriptor = -1;
    int readEndpoint = -1;
    int writeEndpoint = -1;
};
} // namespace RealSenseID
