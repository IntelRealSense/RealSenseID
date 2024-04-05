// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "LicenseUtils_win.h"
#include "../../Logger/Logger.h"
#include <WinReg.hpp>
#include <algorithm>
#include <iterator>
#include <codecvt>

using winreg::RegKey;
using winreg::RegResult;

static const char* LOG_TAG = "LicenseUtils";

namespace RealSenseID
{

static const auto LICENSE_KEY_PATH = L"SOFTWARE\\Intel\\VisionPlatform";
static const auto LICENSE_KEY_NAME = L"License";

#pragma warning(push)
#pragma warning(disable : 4996) // wstring_convert marked deprecated in c++17, but we're still using c++14.
                                // no replacement till c++22, so...
inline std::string getResultString(const RegResult& result)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    auto error_message = converter.to_bytes(result.ErrorMessage());
    return error_message;
}
#pragma warning(pop)

LicenseResult LicenseUtils_win::GetLicenseKey(std::string& license_key)
{
    if (!temp_license_key.empty()) {
        license_key = temp_license_key;
        return {LicenseResult::Status::Ok, "Success"};
    }

    license_key = "";
    RegKey key;
    RegResult result;

    result = key.TryOpen(HKEY_LOCAL_MACHINE, LICENSE_KEY_PATH, KEY_READ);
    if (result.IsOk()) {
        const auto res = key.TryGetStringValue(LICENSE_KEY_NAME);
        if(res.IsValid()) {
            const auto& val = res.GetValue();
            std::transform(val.begin(), val.end(), std::back_inserter(license_key), [] (wchar_t c) {
                return (char)c;
            });
            return {LicenseResult::Status::Ok, "Success"};
        } else {
            auto error_message = "Error while reading registry value: " +getResultString(res.GetError());
            LOG_ERROR(LOG_TAG, error_message.c_str());
            return {LicenseResult::Status::Error, error_message};
        }
    } else {
        auto error_message = "Error while opening registry key: " + getResultString(result);
        LOG_ERROR(LOG_TAG,  error_message.c_str());
        return {LicenseResult::Status::Error, error_message};
    }
}

LicenseResult LicenseUtils_win::SetLicenseKey(const std::string& license_key, bool persist)
{
    if (persist) {
        RegKey key;
        RegResult result;

        result = key.TryOpen(HKEY_LOCAL_MACHINE, LICENSE_KEY_PATH, KEY_WRITE);
        if (result.Failed()) {
            result = key.TryCreate(HKEY_LOCAL_MACHINE, LICENSE_KEY_PATH, KEY_WRITE);
        }
        if (result.IsOk()) {
            std::wstring w_license_key(license_key.length(), L' ');
            std::copy(license_key.begin(), license_key.end(), w_license_key.begin());
            const auto res = key.TrySetStringValue(LICENSE_KEY_NAME, w_license_key);
            if(res.IsOk()) {
                temp_license_key = license_key;
                return {LicenseResult::Status::Ok, "Success"};
            } else {
                auto error_message = "Error setting registry value: " + getResultString(res);
                LOG_ERROR(LOG_TAG, error_message.c_str());
                return {LicenseResult::Status::Error, error_message};
            }
        } else {
            // You need to either have permission to the registry entry or admin access:
            // Elevate the process in GUI:
            // https://learn.microsoft.com/en-us/windows/win32/wmisdk/executing-privileged-operations-using-c-
            auto error_message = "Error while opening/creating registry key: " + getResultString(result);
            LOG_ERROR(LOG_TAG, error_message.c_str());
            return {LicenseResult::Status::Error, error_message};
        }
    } else
    {
        temp_license_key = license_key;
        return {LicenseResult::Status::Ok, "Success"};
    }
}

// TODO: In case we would like the customer to override default endpoint url, we would need
// to implement this method to read from registry as in the example above
std::string LicenseUtils_win::GetLicenseEndpointUrl()
{
    return DEFAULT_LICENSE_SERVER_URL;
}

}
