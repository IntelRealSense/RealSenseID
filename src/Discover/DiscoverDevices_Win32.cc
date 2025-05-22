// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.


#include "RealSenseID/DiscoverDevices.h"
#include "Logger.h"

#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <evr.h>
#include <mfapi.h>
#include <string.h>
#include <string>
#include <sstream>
#include <regex>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cassert>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mf")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "Strmiids")
#pragma comment(lib, "Mfreadwrite")

static const char* LOG_TAG = "DiscoverDevices";

namespace RealSenseID
{
static const std::regex VID_REGEX(".*VID_([0-9A-Fa-f]{4}).*", std::regex_constants::icase);
static const std::regex PID_REGEX(".*PID_([0-9A-Fa-f]{4}).*", std::regex_constants::icase);
static const std::regex COM_PORT_REGEX {".*(COM[0-9]+).*"};
static constexpr size_t BUFFER_SIZE = 4096;

struct DeviceDescriptor
{
    const std::string vid;
    const std::string pid;
    const DeviceType deviceType = DeviceType::Unknown;
};

static const std::vector<DeviceDescriptor> ExpectedVidPidPairs {{"04d8", "00dd", DeviceType::F45x},
                                                                {"2aad", "6373", DeviceType::F45x},
                                                                {"414c", "6578", DeviceType::F46x},
                                                                {"414c", "6666", DeviceType::F46x}};

static bool MatchToExpectedVidPidPairs(std::string vid, std::string pid, DeviceType& deviceType)
{
    deviceType = DeviceType::Unknown;
    std::transform(vid.begin(), vid.end(), vid.begin(), ::tolower);
    std::transform(pid.begin(), pid.end(), pid.begin(), ::tolower);

    for (const auto& expected : ExpectedVidPidPairs)
    {
        if (vid == expected.vid && pid == expected.pid)
        {
            deviceType = expected.deviceType;
            return true;
        }
    }

    assert(deviceType == DeviceType::Unknown);
    return false;
}

static std::string ExtractStringUsingRegex(const std::string& input, const std::regex& regex)
{
    std::smatch matches;
    if (std::regex_match(input, matches, regex) && matches.size() == 2)
        return matches[1];
    return {};
}

static void ThrowIfFailedMSMF(const char* what, HRESULT hr)
{
    if (FAILED(hr))
    {
        std::stringstream err_stream;
        err_stream << what << "MSMF failed with HResult error: " << hr;
        throw std::runtime_error(err_stream.str());
    }
}

// Destroy the device_info_set or throw runtime_error if failed
static void DestroyDeviceInfoList(HDEVINFO device_info_set)
{
    if (!::SetupDiDestroyDeviceInfoList(device_info_set))
    {
        LOG_ERROR(LOG_TAG, "SetupDiDestroyDeviceInfoList() error 0x%08lX", GetLastError());
        throw std::runtime_error("SetupDiDestroyDeviceInfoList() failed");
    }
}

// Return serial ports connected to RealSenseID devices
static std::vector<std::string> DiscoverSerial()
{
    std::vector<std::string> port_names;
    const GUID guid = GUID_DEVCLASS_PORTS;
    HDEVINFO device_info_set = SetupDiGetClassDevs(&guid, nullptr, nullptr, DIGCF_PRESENT);
    if (device_info_set == INVALID_HANDLE_VALUE)
    {
        LOG_ERROR(LOG_TAG, "SetupDiGetClassDevs returned invalid handle");
        return port_names;
    }

    SP_DEVINFO_DATA device_info_data = {};
    device_info_data.cbSize = sizeof(device_info_data);

    for (int i = 0; SetupDiEnumDeviceInfo(device_info_set, i, &device_info_data); ++i)
    {
        char device_id_buffer[BUFFER_SIZE] = {};
        DWORD device_id_size = 0;
        auto ok = SetupDiGetDeviceInstanceIdA(device_info_set, &device_info_data, device_id_buffer, BUFFER_SIZE - 1, &device_id_size);
        if (!ok)
        {
            LOG_ERROR(LOG_TAG, "SetupDiGetDeviceInstanceId() error 0x%08lX", GetLastError());
            continue;
        }

        std::string device_id(device_id_buffer);
        std::string vid = ExtractStringUsingRegex(device_id, VID_REGEX);
        std::string pid = ExtractStringUsingRegex(device_id, PID_REGEX);

        if (vid.empty() || pid.empty())
            continue;

        DeviceType deviceType;
        if (!MatchToExpectedVidPidPairs(vid, pid, deviceType))
            continue;

        BYTE name_buffer[BUFFER_SIZE] = {};
        DWORD name_size = 0;
        ok = ::SetupDiGetDeviceRegistryPropertyA(device_info_set, &device_info_data, SPDRP_FRIENDLYNAME, nullptr, name_buffer,
                                                 BUFFER_SIZE - 1, &name_size);
        if (!ok)
        {
            LOG_ERROR(LOG_TAG, "SetupDiGetDeviceRegistryProperty() error 0x%08lX", GetLastError());
            continue;
        }
        std::string friendly(reinterpret_cast<const char*>(name_buffer));
        std::string com_port = ExtractStringUsingRegex(friendly, COM_PORT_REGEX);
        if (!com_port.empty())
        {
            port_names.push_back(std::move(com_port));
        }
    }
    DestroyDeviceInfoList(device_info_set);
    return port_names;
}

std::vector<int> DiscoverCapture()
{
    std::vector<int> capture_numbers;
    IMFAttributes* cap_config = nullptr;
    IMFActivate** ppDevices = nullptr;
    UINT32 count = 0;

    ThrowIfFailedMSMF("DiscoverCapture", MFCreateAttributes(&cap_config, 10));
    ThrowIfFailedMSMF("DiscoverCapture",
                      cap_config->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID));
    ThrowIfFailedMSMF("DiscoverCapture", MFEnumDeviceSources(cap_config, &ppDevices, &count));

    if (count < 1)
    {
        LOG_ERROR(LOG_TAG, "No video devices detected");
        return capture_numbers;
    }

    for (UINT32 i = 0; i < count; ++i)
    {
        WCHAR* guid = nullptr;
        UINT32 cchName = 0;
        HRESULT hr = ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &guid, &cchName);
        if (SUCCEEDED(hr))
        {
            char buffer[BUFFER_SIZE];
            snprintf(buffer, sizeof(buffer), "%ls", guid);
            std::string device_id(buffer);
            CoTaskMemFree(guid);

            std::string vid = ExtractStringUsingRegex(device_id, VID_REGEX);
            std::string pid = ExtractStringUsingRegex(device_id, PID_REGEX);

            DeviceType deviceType;
            if (!vid.empty() && !pid.empty() && MatchToExpectedVidPidPairs(vid, pid, deviceType))
            {
                capture_numbers.push_back(i);
                LOG_DEBUG(LOG_TAG, "Detected capture device.");
            }
        }
        ppDevices[i]->Release();
    }

    CoTaskMemFree(ppDevices);
    if (cap_config)
        cap_config->Release();
    return capture_numbers;
}

// Discover ports and associate each port to device type
// Return list if devices
std::vector<DeviceInfo> DiscoverDevices()
{
    std::vector<DeviceInfo> devices;
    auto ports = DiscoverSerial();

    for (const auto& port : ports)
    {
        DeviceInfo info;
        ::strcpy_s(info.serialPort, sizeof(info.serialPort), port.c_str());
        info.deviceType = DiscoverDeviceType(info.serialPort);

        if (info.deviceType != DeviceType::Unknown)
            devices.push_back(info);
        else
            LOG_WARNING(LOG_TAG, "Failed to auto detect device type for port: %s", info.serialPort);
    }

    return devices;
}

// Use the port's vid/pid to decide which RealSenseID device is it (F45x, F46x, etc.)
DeviceType DiscoverDeviceType(const char* serial_port)
{
    const GUID guid = GUID_DEVCLASS_PORTS;
    HDEVINFO device_info_set = SetupDiGetClassDevs(&guid, nullptr, nullptr, DIGCF_PRESENT);
    if (device_info_set == INVALID_HANDLE_VALUE)
    {
        LOG_WARNING(LOG_TAG, "Cannot detect device type");
        return DeviceType::Unknown;
    }

    SP_DEVINFO_DATA device_info_data = {};
    device_info_data.cbSize = sizeof(device_info_data);

    std::string upper_port(serial_port);
    std::transform(upper_port.begin(), upper_port.end(), upper_port.begin(), ::toupper);
    upper_port = ExtractStringUsingRegex(upper_port, COM_PORT_REGEX);

    for (int i = 0; SetupDiEnumDeviceInfo(device_info_set, i, &device_info_data); ++i)
    {
        char device_id_buffer[BUFFER_SIZE] = {};
        DWORD device_id_size = 0;
        auto ok = SetupDiGetDeviceInstanceIdA(device_info_set, &device_info_data, device_id_buffer, BUFFER_SIZE - 1, &device_id_size);
        if (!ok)
        {
            LOG_ERROR(LOG_TAG, "SetupDiGetDeviceInstanceIdA() error 0x%08lX", GetLastError());
            continue;
        }
        std::string device_id(reinterpret_cast<const char*>(device_id_buffer));
        std::string vid = ExtractStringUsingRegex(device_id, VID_REGEX);
        std::string pid = ExtractStringUsingRegex(device_id, PID_REGEX);

        DeviceType deviceType;
        if (!MatchToExpectedVidPidPairs(vid, pid, deviceType))
            continue;

        BYTE name_buffer[BUFFER_SIZE] = {};
        DWORD name_size = 0;
        ok = SetupDiGetDeviceRegistryPropertyA(device_info_set, &device_info_data, SPDRP_FRIENDLYNAME, nullptr, name_buffer,
                                               BUFFER_SIZE - 1, &name_size);
        if (!ok)
        {
            LOG_ERROR(LOG_TAG, "SetupDiGetDeviceRegistryPropertyA() error 0x%08lX", GetLastError());
            continue;
        }

        std::string friendly(reinterpret_cast<const char*>(name_buffer));
        std::string friendly_port = ExtractStringUsingRegex(friendly, COM_PORT_REGEX);

        if (upper_port == friendly_port)
        {
            DestroyDeviceInfoList(device_info_set);
            LOG_INFO(LOG_TAG, "Detected device %s", Description(deviceType));
            return deviceType;
        }
    }

    DestroyDeviceInfoList(device_info_set);
    LOG_WARNING(LOG_TAG, "Cannot detect device type");
    return DeviceType::Unknown;
}

} // namespace RealSenseID
