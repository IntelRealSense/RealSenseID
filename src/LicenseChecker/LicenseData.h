// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once


#include <string>

namespace RealSenseID
{

struct LicenseInfoResponse{
    int license_type;
    std::string payload;
};
}
