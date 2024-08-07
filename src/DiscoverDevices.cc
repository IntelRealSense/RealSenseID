// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2024 Intel Corporation. All Rights Reserved.

#include "RealSenseID/DiscoverDevices.h"
#include "Logger.h"
#include <algorithm>
#include <cctype>
#include <utility>
#include <string>
#include <regex>
#include <string>
#include <sstream>
#include <array>
#include <cstring>

static const char* LOG_TAG = "Utilities";


namespace RealSenseID
{
static const std::regex VID_REGEX(".*VID_([0-9A-Fa-f]{4}).*", std::regex_constants::icase);
static const std::regex PID_REGEX(".*PID_([0-9A-Fa-f]{4}).*", std::regex_constants::icase);
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
} // namespace RealSenseID

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

// Throw if given hresult failed
static void ThrowIfFailedMSMF(char* what, HRESULT hr)
{
    if (SUCCEEDED(hr))
        return;
    std::stringstream err_stream;
    err_stream << what << "MSMF failed with  HResult error: " << hr;
    throw std::runtime_error(err_stream.str());
}

namespace RealSenseID
{

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

    for (const auto& port_name : port_names)
    {
        DeviceInfo device = {0};
        ::strncpy(device.serialPort, port_name.c_str(), sizeof(device.serialPort) - 1);
        device.serialPort[sizeof(device.serialPort) - 1] = '\0';
        devices.push_back(device);
    }

    return devices;
}
} // namespace RealSenseID


#elif LINUX

#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <string>
#include <sstream>
#include <tuple>
#ifdef RSID_PREVIEW
#include "libuvc/libuvc.h"
#endif

namespace RealSenseID
{

#ifdef RSID_PREVIEW
inline void ThrowIfFailedUVC(const char* call, uvc_error_t status)
{
    if (status != UVC_SUCCESS)
    {
        std::stringstream err_stream;
        err_stream << call << "(...) failed with: " << uvc_strerror(status);
        throw std::runtime_error(err_stream.str());
    }
}

std::vector<int> DiscoverCapture()
{
    std::vector<int> capture_numbers;

    uvc_context_t* ctx;
    uvc_device_t** list;

    try
    {
        ThrowIfFailedUVC("uvc_init", uvc_init(&ctx, nullptr));
        ThrowIfFailedUVC("uvc_get_device_list", uvc_get_device_list(ctx, &list));

        int index = 0;

        for (auto it = list; *it; ++it)
        {
            uvc_device_descriptor_t* desc;
            try
            {
                ThrowIfFailedUVC("uvc_get_device_descriptor", uvc_get_device_descriptor(*it, &desc));

                bool is_known_device = false;
                for (const auto& expected : ExpectedVidPidPairs)
                {
                    const auto expected_vid = std::stoul(expected.vid, nullptr, 16);
                    const auto expected_pid = std::stoul(expected.pid, nullptr, 16);

                    if (desc->idVendor == expected_vid && desc->idProduct == expected_pid)
                    {
                        capture_numbers.emplace_back(index);
                        LOG_DEBUG(LOG_TAG, "[*] Device at index (%i) with vid: '%04x', pid: '%04x' is F45x device.",
                                  index, desc->idVendor, desc->idProduct);
                        is_known_device = true;
                    }
                }
                if (!is_known_device)
                {
                    LOG_DEBUG(
                        LOG_TAG,
                        "[ ] Device at index (%i) with vid: '%04x', pid: '%04x' is _not_ F45x and is not supported.",
                        index, desc->idVendor, desc->idProduct);
                }
                index++;
                uvc_free_device_descriptor(desc);
            }
            catch (std::runtime_error& e)
            {
                LOG_WARNING(LOG_TAG, e.what());
                if (desc)
                {
                    uvc_free_device_descriptor(desc);
                }
            }
        }
    }
    catch (std::exception& ex)
    {
        LOG_ERROR(LOG_TAG, "Exception in DiscoverCapture: %s", ex.what());
        if (list)
        {
            uvc_free_device_list(list, 1);
        }
        if (ctx)
        {
            uvc_exit(ctx);
        }
    }

    uvc_free_device_list(list, 1);
    if (ctx)
    {
        uvc_exit(ctx);
    }

    LOG_DEBUG(LOG_TAG, "Capture devices: %lu", capture_numbers.size());
    return capture_numbers;
}

#else // RSID_PREVIEW

#pragma message("Note: DiscoverCapture is disabled when RSID_PREVIEW option is disabled")

std::vector<int> DiscoverCapture()
{
    LOG_WARNING(LOG_TAG, "DiscoverCapture is disabled when RSID_PREVIEW option is disabled at build-time.");
    return {};
}

#endif

static std::tuple<bool, std::string, std::string> ParseVidPid(const std::string& uevent_file_path)
{
    std::ifstream uevent_file(uevent_file_path);
    if (!uevent_file)
    {
        LOG_WARNING(LOG_TAG, "Cannot access %s.", uevent_file_path.c_str());
        return {false, "", ""};
    }

    std::string uevent_line;
    std::string vid, pid;

    while (std::getline(uevent_file, uevent_line) && (vid.empty() || pid.empty()))
    {
        if (uevent_line.find("PRODUCT=") != std::string::npos)
        {
            try
            {
                vid = uevent_line.substr(uevent_line.find_last_of('=') + 1, 4);
                pid = uevent_line.substr(uevent_line.find_last_of('=') + 6, 4);
                if ((vid.find('/') != std::string::npos) || (pid.find('/') != std::string::npos))
                {
                    continue;
                }
                // LOG_DEBUG(LOG_TAG, "VID: %s, PID: %s", vid.c_str(), pid.c_str());
            }
            catch (...)
            {
            }
        }
    }

    return {!(vid.empty() || pid.empty()), vid, pid};
}

std::vector<DeviceInfo> DiscoverDevices()
{
    std::vector<DeviceInfo> devices;

    constexpr char BASE_PATH[] = "/sys/bus/usb/devices/";
    DIR* dir = opendir(BASE_PATH);
    if (!dir)
    {
        LOG_ERROR(LOG_TAG, "Cannot access %s", BASE_PATH);
        return devices;
    }

    while (dirent* entry = readdir(dir))
    {
        std::string name = entry->d_name;
        if (name == "." || name == ".." || name.find(':') == std::string::npos)
            continue;

        const std::string path = BASE_PATH + name;
        std::string real_path {};
        char buff[PATH_MAX] = {0};
        if (realpath(path.c_str(), buff) != nullptr)
        {
            real_path = std::string(buff);
            //
            // Determine if it's an F45x device
            //
            bool parse_status;
            std::string vid, pid;
            std::tie(parse_status, vid, pid) = std::move(ParseVidPid(real_path + "/uevent"));
            if (!parse_status || !MatchToExpectedVidPidPairs(vid, pid))
            {
                continue;
            }
            //
            // Passed all identification checks. Find actual /dev entry
            //
            const std::string tty_path = real_path + "/tty";
            DIR* tty_dir = opendir(tty_path.c_str());
            if (!tty_dir)
            {
                continue;
            }

            while (dirent* tty_entry = readdir(tty_dir))
            {
                std::string tty_name = tty_entry->d_name;
                if (tty_name == "." || tty_name == "..")
                    continue;

                // Check device entry
                const std::string tty_dev_path = "/dev/" + tty_name;
                struct stat dev_stat{};
                if (stat(tty_dev_path.c_str(), &dev_stat) != 0 || !S_ISCHR(dev_stat.st_mode))
                {
                    continue;
                }

                // We found a valid device and serial port to match it!
                DeviceInfo device = {0};
                ::strncpy(device.serialPort, tty_dev_path.c_str(), sizeof(device.serialPort) - 1);
                device.serialPort[sizeof(device.serialPort) - 1] = '\0';
                devices.push_back(device);
            }
            closedir(tty_dir);
        }
    }
    closedir(dir);

    return devices;
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
