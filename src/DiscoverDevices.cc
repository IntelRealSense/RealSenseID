// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#include "RealSenseID/DiscoverDevices.h"
#include "Logger.h"

static const char* LOG_TAG = "Utilities";


#include <algorithm>
#include <cctype>
#include <utility>
#include <string>
#include <regex>
#include <string>
#include <sstream>
#include <array>

namespace RealSenseID
{
static const std::regex VID_REGEX (".*VID_([0-9A-Fa-f]{4}).*",std::regex_constants::icase);
static const std::regex PID_REGEX (".*PID_([0-9A-Fa-f]{4}).*",std::regex_constants::icase);
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
}

#if _WIN32

// clang-format off
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <initguid.h>
#include <devpkey.h>
// clang-format on
#pragma comment(lib, "setupapi.lib")
#include <evr.h>
#include <mfapi.h>
#include <mfreadwrite.h>

#pragma comment(lib, "mfplat")
#pragma comment(lib, "mf")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "Strmiids")
#pragma comment(lib, "Mfreadwrite")

namespace RealSenseID
{
static void ThrowIfFailedMSMF(char* what, HRESULT hr)
{
    if (SUCCEEDED(hr))
        return;
    std::stringstream err_stream;
    err_stream << what << "MSMF failed with  HResult error: " << hr;
    throw std::runtime_error(err_stream.str());
}

std::vector<std::string> DiscoverSerial()
{
    std::vector<std::string> port_names;
    // Fetch device information set for needed GUID.
    const GUID guid = GUID_DEVCLASS_PORTS;
    HDEVINFO device_info_set = SetupDiGetClassDevs(&guid, 0, nullptr, DIGCF_PRESENT);
    if (device_info_set == INVALID_HANDLE_VALUE)
        return port_names;

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

        port_names.push_back(com_port_handle_string);
    }

    SetupDiDestroyDeviceInfoList(device_info_set);
    return port_names;
}

std::vector<int> DiscoverCapture()
{
    IMFAttributes* cap_config;
    std::vector<int> capture_numbers;

    static char* stage_tag = "Discover Capture";
    IMFActivate** ppDevices = NULL;
    UINT32 count = 0;
    bool found = false;

    ThrowIfFailedMSMF(stage_tag, MFCreateAttributes(&cap_config, 10));
    ThrowIfFailedMSMF(
        stage_tag,
        (cap_config)->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));
    ThrowIfFailedMSMF(stage_tag, MFEnumDeviceSources((cap_config), &ppDevices, &count));

    if (count < 1)
    {
        LOG_ERROR(LOG_TAG, "no video devices detected");
        return capture_numbers;
    }

    constexpr size_t max_buffer_size = 4096;
    for (UINT32 device_index = 0; device_index < count; device_index++)
    {
        HRESULT hr = S_OK;
        WCHAR* guid = NULL;
        UINT32 cchName;

        hr = ppDevices[device_index]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &guid,
                                                         &cchName);
        if (SUCCEEDED(hr))
        {
            char device_id_buffer[max_buffer_size];
            snprintf(device_id_buffer, max_buffer_size, "%ls", guid);
            std::string device_id_string(device_id_buffer);
            CoTaskMemFree(guid);

            std::string vid = ExtractStringUsingRegex(device_id_string, VID_REGEX);
            std::string pid = ExtractStringUsingRegex(device_id_string, PID_REGEX);


            if (pid.empty() || vid.empty())
                continue;

            auto matchResult = MatchToExpectedVidPidPairs(vid, pid);
            if (matchResult)
            {
                capture_numbers.push_back(device_index);
                found = true;
                LOG_DEBUG(LOG_TAG, "detected capture device.");
            }
        }
         ppDevices[device_index]->Release();
    }
    CoTaskMemFree(ppDevices);
    if (!found)
    {
        LOG_ERROR(LOG_TAG, "Failed to auto detect capture device.");
    }
    return capture_numbers;
}

std::vector<DeviceInfo> DiscoverDevices()
{
    std::vector<DeviceInfo> devices;
    std::vector<std::string> port_names;

    port_names = DiscoverSerial();

    for (unsigned int i = 0; i < port_names.size() ; ++i)
    {
        DeviceInfo device = {0};
        ::strncpy(device.serialPort, port_names[i].c_str(), sizeof(device.serialPort) - 1);
        device.serialPort[sizeof(device.serialPort) - 1] = '\0';
        devices.push_back(device);
    }
    
    return devices;
}
} // namespace RealSenseID


#elif LINUX
namespace RealSenseID
{
static const std::string V4L_PATH = "/sys/class/video4linux/";
static const std::string V4L_CAM_ID_PATH = "/device/*/*/id/";
static const int FAILED_V4L = -1;

static void ThrowIfFailed(char* what, int res)
{
    if (res != FAILED_V4L)
        return;
    std::stringstream err_stream;
    err_stream << what << "v4l failed with error.";
    throw std::runtime_error(err_stream.str());
}

static std::string ExecuteCmd(const std::string cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    if (!result.empty())
    {
        result.pop_back();
    }
    result.pop_back();
    return result;
}

std::vector<int> DiscoverCapture()
{
    std::vector<int> capture_numbers;

    static const std::string id_req = "less " + V4L_PATH + "video";
    static const std::string v4l_num_req = "ls " + V4L_PATH + " | wc -l";

    int res = FAILED_V4L;
    std::string result;

    int number_of_dev = std::stoi(ExecuteCmd(v4l_num_req));
    for (int cur = 0; cur < number_of_dev; cur++)
    {
        std::string vid = ExecuteCmd(id_req + std::to_string(cur) + V4L_CAM_ID_PATH + "vendor");
        std::string pid = ExecuteCmd(id_req + std::to_string(cur) + V4L_CAM_ID_PATH + "product");
        
        if (vid.empty() || pid.empty())
                continue;

        if (MatchToExpectedVidPidPairs(vid, pid))
        {
            capture_numbers.push_back(cur);
        }
    }
    LOG_DEBUG(LOG_TAG, " capture devices %d",capture_numbers.size());
    return capture_numbers;
}

std::vector<DeviceInfo> DiscoverDevices()
{
    LOG_DEBUG(LOG_TAG, "DiscoverDevices is not implemented on this platform!");
    return {};
}
} // namespace RealSenseID
#else
namespace RealSenseID
{
std::vector<int> DiscoverCapture()
{
    LOG_DEBUG(LOG_TAG, "DiscoverCapture is not implemented on this platform!");
    return {};
}

std::vector<DeviceInfo> DiscoverDevices()
{
    LOG_DEBUG(LOG_TAG, "DiscoverDevices is not implemented on this platform!");
    return {};
}
} // namespace RealSenseID
#endif