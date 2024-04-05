// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include <vector>
#include "LicenseUtils.h"
#include "base64.hpp"
#if defined(_WIN32) || defined(_WIN64)
#   include "LicenseUtilsImpl/LicenseUtils_win.h"
#elif defined(ANDROID)
#   include "LicenseUtilsImpl/LicenseUtils_android.h"
#else
#   include "LicenseUtilsImpl/LicenseUtils_nix.h"
#endif

namespace RealSenseID
{

LicenseUtils::LicenseUtils()
{
#if defined(_WIN32) || defined(_WIN64)
    licenseManagerImpl = std::make_unique<LicenseUtils_win>();
#elif defined(ANDROID)
    licenseManagerImpl = std::make_unique<LicenseUtils_android>();
#else // Unix/Linux
    licenseManagerImpl = std::make_unique<LicenseUtils_nix>();
#endif
}

LicenseResult LicenseUtils::GetLicenseKey(std::string& license_key)
{
    return licenseManagerImpl->GetLicenseKey(license_key);
}

std::string LicenseUtils::GetLicenseEndpointUrl() {
    return licenseManagerImpl->GetLicenseEndpointUrl();
}

LicenseResult LicenseUtils::SetLicenseKey(const std::string& license_key, bool persist)
{
    return licenseManagerImpl->SetLicenseKey(license_key, persist);
}

std::string LicenseUtils::Base64Encode(const std::vector<unsigned char>& vec) {
    return base64::encode(vec);
}

std::vector<unsigned char> LicenseUtils::Base64Decode(const std::string& str) {
    return base64::decode(str);
}

}
