// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <fstream>
#include <memory>
#include <stdexcept>
#include <future>
#include "LicenseUtils.h"
#include "LicenseCheckerImpl/ILicenseCheckerImpl.h"

#ifndef LICENSE_SIGNATURE_SIZE
    #define LICENSE_VERIFICATION_RES_SIZE 64
    #define LICENSE_SIGNATURE_SIZE        384
#endif // !LICENSE_SIGNATURE_SIZE


namespace RealSenseID
{

enum LicenseType
{
    NO_FEATURES = 0,
    ANTI_SPOOF_SUBSCRIPTION = 1,
    FACIAL_AUTH_SUBSCRIPTION = 2,
    ANTI_SPOOF_RENEWAL = 3,
    FACIAL_AUTH_RENEWAL = 4,
    ANTI_SPOOF_PERPETUAL = 5,
    FACIAL_AUTH_PERPETUAL = 6,
};

class LicenseChecker final : public Singleton<LicenseChecker>
{
public:
    LicenseChecker();
    LicenseCheckStatus CheckLicense(const unsigned char* data, unsigned char* payload, int& license_type);

    LicenseCheckStatus CheckLicense(const std::vector<unsigned char>& iv,
                                    const std::vector<unsigned char>& enc_session_token,
                                    const std::vector<unsigned char>& serial_number, unsigned char* payload,
                                    int& license_type);

private:
    std::unique_ptr<ILicenseCheckerImpl> licenseCheckerImpl;

    friend Singleton;
};

} // namespace RealSenseID
