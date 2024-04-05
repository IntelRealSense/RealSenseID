// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "LicenseChecker.h"

// TODO: Figure out a way to share those defines between host/secure/non-secure/device
#define AES_CTR_IV_SIZE_BYTES          16
#define LICENSE_SESSION_TOKEN_SIZE     16
#define SERIAL_NUMBER_SIZE             28

#ifdef RSID_MOCK_LICENSE
#  include <LicenseCheckerImpl/LicenseCheckerMockImpl.h>
#else
#  include "LicenseCheckerImpl/LicenseCheckerImpl.h"
#endif

namespace RealSenseID
{

LicenseChecker::LicenseChecker()
{
#ifdef RSID_MOCK_LICENSE
    licenseCheckerImpl = std::make_unique<LicenseCheckerMockImpl>();
#else
    licenseCheckerImpl = std::make_unique<LicenseCheckerImpl>();
#endif
}

LicenseCheckStatus LicenseChecker::CheckLicense(const std::vector<unsigned char>& iv,
                                                const std::vector<unsigned char>& enc_session_token,
                                                const std::vector<unsigned char>& serial_number,
                                                unsigned char* payload, int& license_type)
{
    return licenseCheckerImpl->CheckLicense(iv, enc_session_token, serial_number, payload, license_type);
}

LicenseCheckStatus LicenseChecker::CheckLicense(const unsigned char* data, unsigned char* payload, int& license_type) {
    std::vector<unsigned char> iv(data,
                                  data + AES_CTR_IV_SIZE_BYTES);
    std::vector<unsigned char> enc_session_token(data + AES_CTR_IV_SIZE_BYTES,
                                                 data + AES_CTR_IV_SIZE_BYTES + LICENSE_SESSION_TOKEN_SIZE);
    std::vector<unsigned char> serial_number(data + AES_CTR_IV_SIZE_BYTES + LICENSE_SESSION_TOKEN_SIZE,
                                             data + AES_CTR_IV_SIZE_BYTES + LICENSE_SESSION_TOKEN_SIZE+ SERIAL_NUMBER_SIZE);

    // payload: | iv[16] | enc_data[64] (key: shared_key)
    // data: | license_type[1] | session_token[16] | serial_number[28]
    return CheckLicense(iv, enc_session_token, serial_number, payload, license_type);
}

}