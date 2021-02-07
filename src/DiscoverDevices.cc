#if _WIN32
#include "RealSenseID/DiscoverDevices.h"

#include "RealSenseID/DeviceController.h"
#include "Logger.h"

// clang-format off
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <initguid.h>
#include <devpkey.h>
// clang-format on

#include <algorithm>
#include <utility>
#include <vector>
#include <string>
#include <regex>
#include <string>
#include <sstream>

#pragma comment(lib, "setupapi.lib")

namespace RealSenseID
{
static const char* LOG_TAG = "Utilities";

static const std::regex VID_REGEX {".*VID_([0-9A-Fa-f]{4}).*"};
static const std::regex PID_REGEX {".*PID_([0-9A-Fa-f]{4}).*"};
static const std::regex COM_PORT_REGEX {".*(COM[0-9]+).*"};

static const std::vector<std::pair<RealSenseID::SerialType, std::pair<std::string, std::string>>> ExpectedVidPidPairs {
    std::make_pair(RealSenseID::SerialType::UART, std::make_pair("04d8", "00dd")),
    std::make_pair(RealSenseID::SerialType::USB, std::make_pair("2aad", "6373")),
};

// Matches given VID/PID pairs to expected ones and returns {true, RealSenseID::SerialType} if found, and {false,
// RealSenseID::SerialType} otherwise.
static std::pair<bool, RealSenseID::SerialType> MatchToExpectedVidPidPairs(std::string vid, std::string pid)
{
    // Normalize input VID/PID to lower case.
    std::transform(vid.begin(), vid.end(), vid.begin(), [](unsigned char c) { return std::tolower(c); });
    std::transform(pid.begin(), pid.end(), pid.begin(), [](unsigned char c) { return std::tolower(c); });

    for (const auto& expected : ExpectedVidPidPairs)
    {
        const auto& serial_type = expected.first;
        const auto& expected_vid = expected.second.first;
        const auto& expected_pid = expected.second.second;

        if (vid == expected_vid && pid == expected_pid)
            return std::make_pair(true, serial_type);
    }

    return std::make_pair(false, RealSenseID::SerialType::USB);
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

        auto matchResult = MatchToExpectedVidPidPairs(vid, pid);
        if (!matchResult.first)
            continue;

        auto serial_type = matchResult.second;

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
        serial_config.serType = serial_type;

        RealSenseID::DeviceController deviceController;
        auto connect_status = deviceController.Connect(serial_config);
        if (connect_status != RealSenseID::Status::Ok)
        {
            LOG_DEBUG(LOG_TAG, "Failed connecting to device on port %s", serial_config.port);
            continue;
        }

        std::string fw_version;
        deviceController.QueryFirmwareVersion(fw_version);
        if (fw_version.empty())
        {
            LOG_DEBUG(LOG_TAG, "Failed retrieving firmware version, skipping device");
            deviceController.Disconnect();
            continue;
        }

        std::string serial_number;
        deviceController.QuerySerialNumber(serial_number);
        if (serial_number.empty())
        {
            LOG_DEBUG(LOG_TAG, "Failed retrieving serial number, skipping device");
            deviceController.Disconnect();
            continue;
        }
        
        deviceController.Disconnect();

        if (fw_version.size() >= DeviceInfo::MaxBufferSize || serial_number.size() >= DeviceInfo::MaxBufferSize ||
            com_port_handle_string.size() >= DeviceInfo::MaxBufferSize)
        {
            LOG_DEBUG(LOG_TAG, "DeviceInfo buffers too small. Allocated: %zu, Actual (fw, sn, com): %zu, %zu, %zu",
                      DeviceInfo::MaxBufferSize, fw_version.size(), serial_number.size(),
                      com_port_handle_string.size());
            continue;
        }

        DeviceInfo device;
        ::strncpy(device.firmwareVersion, fw_version.c_str(), sizeof(device.firmwareVersion));
        ::strncpy(device.serialNumber, serial_number.c_str(), sizeof(device.serialNumber));
        ::strncpy(device.serialPort, com_port_handle_string.c_str(), sizeof(device.serialPort));
        device.serialPortType = serial_type;

        devices.push_back(device);
    }

    SetupDiDestroyDeviceInfoList(device_info_set);

    return devices;
}
} // namespace RealSenseID

#else

#error Utilities are implemented for Windows only at the moment, aborting compilation.

#endif