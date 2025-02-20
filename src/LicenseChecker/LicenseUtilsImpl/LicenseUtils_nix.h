// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once
#include "ILicenseUtils.h"

namespace RealSenseID
{

class LicenseUtils_nix final : public ILicenseUtils
{
public:
    LicenseResult GetLicenseKey(std::string& license_key) override;
    LicenseResult SetLicenseKey(const std::string& license_key, bool persist) override;
    std::string GetLicenseEndpointUrl() override;

private:
    std::string temp_license_key;
};

} // namespace RealSenseID