// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once
#include <iostream>
#include <string>
#include <utility>
#include <cstdint>
#include "RealSenseID/RealSenseIDExports.h"
#include "RealSenseID/SerialConfig.h"
#include "RealSenseID/Status.h"

namespace RealSenseID
{

namespace UpdateCheck
{
struct RSID_API ReleaseInfo
{
    uint64_t sw_version = 0;
    uint64_t fw_version = 0;
    const char* sw_version_str = nullptr;
    const char* fw_version_str = nullptr;
    const char* release_url = nullptr;
    const char* release_notes_url = nullptr;

    ~ReleaseInfo()
    {
        delete[] sw_version_str;
        delete[] fw_version_str;
        delete[] release_url;
        delete[] release_notes_url;
    }
};

class RSID_API UpdateChecker
{
public:
    UpdateChecker() = default;
    ~UpdateChecker() = default;

    Status GetRemoteReleaseInfo(ReleaseInfo& release_info) const;
    Status GetLocalReleaseInfo(const RealSenseID::SerialConfig& serial_config, ReleaseInfo& release_info) const;
};

} // namespace UpdateCheck
} // namespace RealSenseID
