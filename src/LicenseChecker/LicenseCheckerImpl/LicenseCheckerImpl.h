// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <mutex>
#include <vector>
#include "ILicenseCheckerImpl.h"

namespace RealSenseID
{

class LicenseCheckerImpl : public ILicenseCheckerImpl
{
public:
    LicenseCheckStatus CheckLicense(const std::vector<unsigned char>& iv, const std::vector<unsigned char>& enc_session_token,
                                    const std::vector<unsigned char>& serial_number, unsigned char* payload, int& license_type) override;
};

} // namespace RealSenseID
