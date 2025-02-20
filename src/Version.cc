// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/Version.h"

#include <regex>
#include <sstream>

namespace RealSenseID
{
const char* Version()
{
    static std::string version =
        std::string(std::to_string(RSID_VER_MAJOR) + '.' + std::to_string(RSID_VER_MINOR) + '.' + std::to_string(RSID_VER_PATCH));
    return version.c_str();
}
const char* CompatibleFirmwareVersion()
{
    static std::string version = std::string(std::to_string(RSID_FW_VER_MAJOR) + '.' + std::to_string(RSID_FW_VER_MINOR));

    return version.c_str();
}

bool IsFwCompatibleWithHost(const std::string& fw_version)
{
    std::string search_in = fw_version;

    // firmware version api returns a single version string for all modules
    // this is a temp measure to first extract the relevant firmware module string
    std::stringstream version_stream(fw_version);
    std::string section;
    while (std::getline(version_stream, section, '|'))
    {
        if (section.find("OPFW:") != section.npos)
        {
            auto pos = section.find(":");
            auto sub = section.substr(pos + 1, section.npos);
            search_in = sub;
        }
    }

    // try to extract semver from string
    std::smatch matches;
    std::regex SEMVER_REGEX("^([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)$");
    bool success = std::regex_match(search_in, matches, SEMVER_REGEX);

    if (!success)
        return false;

    auto version_major = std::stoi(matches[1]);
    auto version_minor = std::stoi(matches[2]);

    return version_major == RSID_FW_VER_MAJOR && version_minor >= RSID_FW_VER_MINOR;
}
} // namespace RealSenseID