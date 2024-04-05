// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "LicenseUtils_android.h"
#include <stdexcept>


namespace RealSenseID
{

LicenseResult LicenseUtils_android::GetLicenseKey(std::string& license_key)
{
    if (!_temp_license_key.empty()) {
        license_key = _temp_license_key;
        return {LicenseResult::Status::Ok, "Success"};
    } else
    {
        return {LicenseResult::Status::Error, "License key was not set using SetLicenseKey"};
    }
}

std::string LicenseUtils_android::GetLicenseEndpointUrl() {
    return DEFAULT_LICENSE_SERVER_URL;
}

LicenseResult LicenseUtils_android::SetLicenseKey(const std::string& license_key, bool persist)
{
    if (persist) {
        return {LicenseResult::Status::Error, "Persisting the key is not available for Android"};
    } else {
        _temp_license_key = license_key;
        return {LicenseResult::Status::Ok, "Success"};
    }
}

}
