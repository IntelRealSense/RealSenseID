// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#include "RealSenseID/DiscoverDevices.h"
#include "Logger.h"

static const char* LOG_TAG = "Utilities";

#if _WIN32

// clang-format off
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <initguid.h>
#include <devpkey.h>
// clang-format on

#include <algorithm>
#include <cctype>
#include <utility>
#include <string>
#include <regex>
#include <string>
#include <sstream>

#pragma comment(lib, "setupapi.lib")

namespace RealSenseID
{
static const std::regex VID_REGEX {".*VID_([0-9A-Fa-f]{4}).*"};
static const std::regex PID_REGEX {".*PID_([0-9A-Fa-f]{4}).*"};
static const std::regex COM_PORT_REGEX {".*(COM[0-9]+).*"};

struct DeviceDescriptor
{
    const std::string vid;
    const std::string pid;
};

static const std::vector<DeviceDescriptor> ExpectedVidPidPairs {DeviceDescriptor {"04d8", "00dd"},
                                                                DeviceDescriptor {"2aad", "6373"}};

// Matches given VID/PID pairs to expected ones and returns true if they match the device.
static bool MatchToExpectedVidPidPairs(std::string vid, std::string pid)
{
    // Normalize input VID/PID to lower case.
    std::transform(vid.begin(), vid.end(), vid.begin(), [](unsigned char c) { return std::tolower(c); });
    std::transform(pid.begin(), pid.end(), pid.begin(), [](unsigned char c) { return std::tolower(c); });

    for (const auto& expected : ExpectedVidPidPairs)
    {
        const auto& expected_vid = expected.vid;
        const auto& expected_pid = expected.pid;

        if (vid == expected_vid && pid == expected_pid)
            return true;
    }

    return false;
}

// Extracts string from input using regex matching - only one submatch is allowed. Returns an empty string on failure.
static std::string ExtractStringUsingRegex(const std::string& input, const std::regex& regex)
{
    std::string output;
    std::smatch matches;
    bool success = std::regex_match(input, matches, regex);
    if (success && matches.size() == 2)
        output = matches[1];

    return output;
}

std::vector<DeviceInfo> DiscoverDevices()
{
    std::vector<DeviceInfo> devices;

    // Fetch device information set for needed GUID.
    const GUID guid = GUID_DEVCLASS_PORTS;
    HDEVINFO device_info_set = SetupDiGetClassDevs(&guid, 0, nullptr, DIGCF_PRESENT);
    if (device_info_set == INVALID_HANDLE_VALUE)
        return devices;

    // Iterate over relevant devices.
    SP_DEVINFO_DATA device_info_data = {0};
    device_info_data.cbSize = sizeof(device_info_data);
    for (int device_index = 0; SetupDiEnumDeviceInfo(device_info_set, device_index, &device_info_data) != 0;
         ++device_index)
    {
        constexpr size_t max_buffer_size = 4096;

        TCHAR device_id_buffer[max_buffer_size];
        DWORD device_id_size = 0;
        SetupDiGetDeviceInstanceId(device_info_set, &device_info_data, device_id_buffer, sizeof(device_id_buffer),
                                   &device_id_size);
        device_id_buffer[device_id_size] = '\0';

        // Assuming project uses multibyte charset, so TCHAR isn't unicode.
        std::string device_id_string(reinterpret_cast<const char*>(device_id_buffer));
        std::string vid = ExtractStringUsingRegex(device_id_string, VID_REGEX);
        std::string pid = ExtractStringUsingRegex(device_id_string, PID_REGEX);

        if (pid.empty() || vid.empty())
            continue;

        if (!MatchToExpectedVidPidPairs(vid, pid))
            continue;

        BYTE friendly_name_buffer[max_buffer_size];
        DWORD friendly_name_size;
        SetupDiGetDeviceRegistryProperty(device_info_set, &device_info_data, SPDRP_FRIENDLYNAME, nullptr,
                                         friendly_name_buffer, sizeof(friendly_name_buffer), &friendly_name_size);
        friendly_name_buffer[friendly_name_size] = '\0';

        // Assuming project uses multibyte charset, so TCHAR isn't unicode.
        std::string friendly_name(reinterpret_cast<const char*>(friendly_name_buffer));
        std::string com_port_string = ExtractStringUsingRegex(friendly_name, COM_PORT_REGEX);

        std::ostringstream oss;
        oss << "\\\\.\\" << com_port_string;
        auto com_port_handle_string = oss.str();

        RealSenseID::SerialConfig serial_config;
        serial_config.port = com_port_handle_string.c_str();

        DeviceInfo device = {0};

        ::strncpy(device.serialPort, com_port_handle_string.c_str(), sizeof(device.serialPort) - 1);
        device.serialPort[sizeof(device.serialPort) - 1] = '\0';

        devices.push_back(device);
    }

    SetupDiDestroyDeviceInfoList(device_info_set);

    return devices;
}
} // namespace RealSenseID

#else
namespace RealSenseID
{
std::vector<DeviceInfo> DiscoverDevices()
{
    LOG_DEBUG(LOG_TAG, "DiscoverDevices is not implemented on this platform!");
    return {};
}
} // namespace RealSenseID
#endif