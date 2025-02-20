// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "LicenseUtils_nix.h"
#include "../../Logger/Logger.h"
#include <unistd.h>
#include <sys/types.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using json = NLOHMANN_JSON_NAMESPACE::json;
using namespace NLOHMANN_JSON_NAMESPACE::literals;

static const char* LOG_TAG = "LicenseUtils";

namespace RealSenseID
{

const auto LICENSE_KEY_PATH_USER = ".intel/visionplatform/license.json"; // ~/.intel/visionplatform/license.json
const auto LICENSE_KEY_PATH_SYSTEM = "/etc/intel/visionplatform/license.json";

struct JsonLicenseFile
{
    std::string license_key;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JsonLicenseFile, license_key)

inline bool file_exists(const std::string& name)
{
    return (access(name.c_str(), F_OK) != -1);
}

inline bool file_is_readable(const std::string& name)
{
    return (access(name.c_str(), R_OK) != -1);
}

LicenseResult LicenseUtils_nix::GetLicenseKey(std::string& license_key)
{
    // If license key was set by SetLicenseKey, use it
    if (!temp_license_key.empty())
    {
        license_key = temp_license_key;
        return {LicenseResult::Status::Ok, "Success"};
    }

    // First - try to find user specific file
    std::string config_file = "";

    std::string home_dir = secure_getenv("HOME");
    auto user_config_file = home_dir + "/" + LICENSE_KEY_PATH_USER;

    license_key = ""; // Initialize
    if (file_exists(user_config_file))
    {
        if (file_is_readable(user_config_file))
        {
            config_file = user_config_file;
        }
        else
        {
            return {LicenseResult::Status::Error, "User license key file is present but process cannot read it. Check file permissions."};
        }
    }
    else if (file_exists(LICENSE_KEY_PATH_SYSTEM))
    {
        if (file_is_readable(LICENSE_KEY_PATH_SYSTEM))
        {
            config_file = LICENSE_KEY_PATH_SYSTEM;
        }
        else
        {
            return {LicenseResult::Status::Error, "System license key file is present but process cannot read it. Check file permissions."};
        }
    }
    else
    {
        LOG_ERROR(LOG_TAG,
                  "Looked in %s for user license.\n"
                  "Looked in %s for system license.\n"
                  "Neither user nor system license key file not present.",
                  user_config_file.c_str(), LICENSE_KEY_PATH_SYSTEM);
        return {LicenseResult::Status::Error, "Neither user nor system license key file not present."};
    }

    try
    {
        LOG_DEBUG(LOG_TAG, "Reading license file from: %s", config_file.c_str());
        std::ifstream ifs(config_file);
        json jf = json::parse(ifs);
        JsonLicenseFile json_license = jf.template get<JsonLicenseFile>();
        license_key = json_license.license_key;
        LOG_TRACE(LOG_TAG, "license_key: %s", license_key.c_str());
        return {LicenseResult::Status::Ok, "Success"};
    }
    catch (const json::exception& ex)
    {
        LOG_ERROR(LOG_TAG, "JSON parser error while parsing license file. Exception: %s", ex.what());
        return {LicenseResult::Status::Error, "License key JSON file format exception."};
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR(LOG_TAG, "Error while processing license file. Exception: %s", ex.what());
        return {LicenseResult::Status::Error, "License key file is present but process cannot read it."};
    }
}

// TODO: In case we would like the customer to override default endpoint url, we would need
// to implement this method to read from config file
std::string LicenseUtils_nix::GetLicenseEndpointUrl()
{
    return DEFAULT_LICENSE_SERVER_URL;
}

LicenseResult LicenseUtils_nix::SetLicenseKey(const std::string& license_key, bool persist)
{
    if (persist)
    {
        throw std::runtime_error("Not implemented");
    }
    else
    {
        temp_license_key = license_key;
        return {LicenseResult::Status::Ok, "Success"};
    }
}

} // namespace RealSenseID
