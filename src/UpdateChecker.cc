#include "RealSenseID/UpdateChecker.h"
#include "RealSenseID/Version.h"
#include "Logger.h"
#include "RealSenseID/DeviceController.h"
#include <nlohmann/json.hpp>
#include <restclient-cpp/restclient.h>
#include <iostream>
#include <sstream>
#include <regex>
#include <cstdint>


using json = NLOHMANN_JSON_NAMESPACE::json;
using namespace NLOHMANN_JSON_NAMESPACE::literals;

static const char* LOG_TAG = "UpdateChecker";

namespace RealSenseID
{
using UpdateCheck::UpdateChecker;

namespace UpdateCheck
{
struct ReleaseInfoInternal {
    uint64_t sw_version = 0;
    uint64_t fw_version = 0;
    std::string sw_version_str;
    std::string fw_version_str;
    std::string release_url;
    std::string release_notes_url;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ReleaseInfoInternal,
                                   sw_version,
                                   fw_version,
                                   sw_version_str,
                                   fw_version_str,
                                   release_url,
                                   release_notes_url
)
}

static const std::string RELEASE_ADDRESS = "https://raw.githubusercontent.com/IntelRealSense/RealSenseID/master/release_info.json";

inline const char* string_to_char_p(const std::string& src) {
    size_t length = strlen(src.c_str());
    const char * dst = new char[length + 1];
    ::memcpy((void*)dst, src.c_str(), length + 1);
    return dst;
}

Status UpdateCheck::UpdateChecker::GetRemoteReleaseInfo(ReleaseInfo& release_info) const
{
    RestClient::Response response;
    try
    {
        response = RestClient::get(RELEASE_ADDRESS);
    } catch (const std::exception& ex) {
        LOG_ERROR(LOG_TAG, "Error while sending license request. Exception: %s", ex.what());
        return Status::Error;
    }

    if (response.code != 200) {
        std::stringstream error;
        error << "Response code: [" << response.code << "]" << std::endl;
        error << "Response body: [" << response.body << "]" << std::endl;
        LOG_ERROR(LOG_TAG, error.str().c_str());
        return Status::Error;
    }

    try
    {
        json j = json::parse(response.body);
        ReleaseInfoInternal release_info_internal = j.template get<ReleaseInfoInternal>();
        release_info.sw_version = release_info_internal.sw_version;
        release_info.fw_version = release_info_internal.fw_version;

        release_info.fw_version_str = string_to_char_p(release_info_internal.fw_version_str);
        release_info.sw_version_str = string_to_char_p(release_info_internal.sw_version_str);
        release_info.release_url = string_to_char_p(release_info_internal.release_url);
        release_info.release_notes_url = string_to_char_p(release_info_internal.release_notes_url);
    } catch (const std::exception& ex) {
        LOG_ERROR(LOG_TAG, "Error while parsing json. Exception: %s", ex.what());
        return Status::Error;
    }

    return Status::Ok;
}

Status UpdateCheck::UpdateChecker::GetLocalReleaseInfo(const RealSenseID::SerialConfig& serial_config,
                                                       ReleaseInfo& release_info) const
{
    RealSenseID::DeviceController deviceController;
    auto connect_status = deviceController.Connect(serial_config);
    if (connect_status != RealSenseID::Status::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed connecting to port %s, status: %d", serial_config.port, connect_status);
        return connect_status;
    }

    std::string firmware_version;
    auto status = deviceController.QueryFirmwareVersion(firmware_version);
    deviceController.Disconnect();
    if (status != RealSenseID::Status::Ok)
    {
        LOG_ERROR(LOG_TAG, "Failed getting firmware version. Status: %d", status);        
        return status;
    }
    
    std::string search_in = firmware_version;

    std::stringstream version_stream(firmware_version);
    std::string section;
    while (std::getline(version_stream, section, '|'))
    {
        if (section.find("OPFW:") != std::string::npos)
        {
            auto pos = section.find(':');
            auto sub = section.substr(pos + 1, std::string::npos);
            search_in = sub;
        }
    }

    // try to extract version from string
    std::smatch matches;
    std::regex SEMVER_REGEX(R"(^([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)$)");
    bool success = std::regex_match(search_in, matches, SEMVER_REGEX);

    if (!success)
    {
        LOG_ERROR(LOG_TAG, "Failed parsing firmware version from string");        
        return Status::Error;
    }
        
    auto fw_major = std::stoul(matches[1]);
    auto fw_minor = std::stoul(matches[2]);
    auto fw_revision = std::stoul(matches[3]);
    auto fw_build = std::stoul(matches[4]);

    release_info.fw_version = fw_build +                // 3 digits for build
                              fw_revision * 10000 +     // 2 digits for revision
                              fw_minor    * 1000000 +   // 2 digits for minor
                              fw_major    * 100000000;  // 2 digits for major
    release_info.fw_version_str = string_to_char_p(search_in);

    release_info.sw_version = RSID_VERSION;
    std::stringstream sw_version_str;
    sw_version_str << RSID_VER_MAJOR << "." << RSID_VER_MINOR << "." << RSID_VER_PATCH;
    release_info.sw_version_str = string_to_char_p(sw_version_str.str());


    return Status::Ok;
}
}
