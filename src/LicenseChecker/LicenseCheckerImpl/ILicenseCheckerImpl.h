// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once
#include <string>
#include <utility>
#include "../PacketManager/CommonTypes.h"


namespace RealSenseID
{

enum class LicenseCheckStatus {
    SUCCESS = 100,               /**< Operation succeeded */
    NetworkError,                /**< Network error */
    Error,                       /**< General error */
};


class ILicenseCheckerImpl
{
public:
    virtual ~ILicenseCheckerImpl() = default;
    virtual LicenseCheckStatus CheckLicense(const std::vector<unsigned char>& iv,
                                            const std::vector<unsigned char>& enc_session_token,
                                            const std::vector<unsigned char>& serial_number,
                                            unsigned char* payload, int& license_type) = 0;
};

}