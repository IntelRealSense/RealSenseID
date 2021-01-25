// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/Version.h"
#include <string>

namespace RealSenseID
{
const char* Version()
{
    static std::string version = std::string(std::to_string(RSID_VER_MAJOR) + '.' + std::to_string(RSID_VER_MINOR) +
                                             '.' + std::to_string(RSID_VER_PATCH));
    return version.c_str();
}
} // namespace RealSenseID