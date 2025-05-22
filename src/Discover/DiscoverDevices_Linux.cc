// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.


#include "RealSenseID/DiscoverDevices.h"
#include "Logger.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <string>
#include <vector>
#include <tuple>
#include <dirent.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <stdexcept>

#ifdef RSID_PREVIEW
#include "libuvc/libuvc.h"
#endif

static const char* LOG_TAG = "DiscoverDevices";

namespace RealSenseID
{

static const std::regex VID_REGEX(".*VID_([0-9A-Fa-f]{4}).*", std::regex_constants::icase);
static const std::regex PID_REGEX(".*PID_([0-9A-Fa-f]{4}).*", std::regex_constants::icase);

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

    return false;
}

static std::tuple<bool, std::string, std::string> ParseVidPid(const std::string& uevent_file_path)
{
    std::ifstream uevent_file(uevent_file_path);
    if (!uevent_file)
    {
        LOG_WARNING(LOG_TAG, "Cannot access %s.", uevent_file_path.c_str());
        return {false, "", ""};
    }

    std::string line;
    std::string vid, pid;

    while (std::getline(uevent_file, line) && (vid.empty() || pid.empty()))
    {
        if (line.find("PRODUCT=") != std::string::npos)
        {
            try
            {
                vid = line.substr(line.find_last_of('=') + 1, 4);
                pid = line.substr(line.find_last_of('=') + 6, 4);

                if (vid.find('/') != std::string::npos || pid.find('/') != std::string::npos)
                    continue;
            }
            catch (...)
            {
                // swallow
            }
        }
    }

    return {!(vid.empty() || pid.empty()), vid, pid};
}

DeviceType DiscoverDeviceType(const char* serial_port)
{
    if (strncmp(serial_port, "/dev/", 5) != 0)
    {
        LOG_WARNING(LOG_TAG, "Cannot detect device type. port must start with /dev/");
        return DeviceType::Unknown;
    }

    std::string dev_path = "/sys/class/tty/" + std::string(serial_port).substr(5) + "/device/uevent";

    bool ok = false;
    std::string vid, pid;
    std::tie(ok, vid, pid) = ParseVidPid(dev_path);
    if (!ok)
    {
        LOG_WARNING(LOG_TAG, "Cannot detect device type");
        return DeviceType::Unknown;
    }

    DeviceType deviceType;
    if (!MatchToExpectedVidPidPairs(vid, pid, deviceType))
    {
        LOG_WARNING(LOG_TAG, "Cannot detect device type (MatchToExpectedVidPidPairs)");
        return DeviceType::Unknown;
    }

    LOG_INFO(LOG_TAG, "Detected device type %s", Description(deviceType));
    return deviceType;
}

#ifdef RSID_PREVIEW

inline void ThrowIfFailedUVC(const char* call, uvc_error_t status)
{
    if (status != UVC_SUCCESS)
    {
        std::stringstream err;
        err << call << "(...) failed with: " << uvc_strerror(status);
        throw std::runtime_error(err.str());
    }
}

std::vector<int> DiscoverCapture()
{
    std::vector<int> capture_numbers;
    uvc_context_t* ctx = nullptr;
    uvc_device_t** list = nullptr;

    try
    {
        ThrowIfFailedUVC("uvc_init", uvc_init(&ctx, nullptr));
        ThrowIfFailedUVC("uvc_get_device_list", uvc_get_device_list(ctx, &list));

        int index = 0;

        for (auto it = list; *it; ++it)
        {
            uvc_device_descriptor_t* desc = nullptr;
            try
            {
                ThrowIfFailedUVC("uvc_get_device_descriptor", uvc_get_device_descriptor(*it, &desc));

                bool matched = false;
                for (const auto& expected : ExpectedVidPidPairs)
                {
                    const auto expected_vid = std::stoul(expected.vid, nullptr, 16);
                    const auto expected_pid = std::stoul(expected.pid, nullptr, 16);

                    if (desc->idVendor == expected_vid && desc->idProduct == expected_pid)
                    {
                        capture_numbers.push_back(index);
                        matched = true;
                        break;
                    }
                }

                if (!matched)
                {
                    LOG_DEBUG(LOG_TAG, "Device at index %i not supported", index);
                }

                ++index;
                uvc_free_device_descriptor(desc);
            }
            catch (...)
            {
                if (desc)
                    uvc_free_device_descriptor(desc);
            }
        }
    }
    catch (std::exception& e)
    {
        LOG_ERROR(LOG_TAG, "DiscoverCapture exception: %s", e.what());
    }

    if (list)
        uvc_free_device_list(list, 1);
    if (ctx)
        uvc_exit(ctx);

    return capture_numbers;
}

#else // RSID_PREVIEW

std::vector<int> DiscoverCapture()
{
    LOG_WARNING(LOG_TAG, "DiscoverCapture is disabled when RSID_PREVIEW is disabled.");
    return {};
}

#endif // RSID_PREVIEW

std::vector<DeviceInfo> DiscoverDevices()
{
    std::vector<DeviceInfo> devices;
    constexpr const char* base_path = "/sys/bus/usb/devices/";
    DIR* dir = opendir(base_path);
    if (!dir)
    {
        LOG_ERROR(LOG_TAG, "Cannot open %s", base_path);
        return devices;
    }

    while (dirent* entry = readdir(dir))
    {
        std::string name = entry->d_name;
        if (name == "." || name == ".." || name.find(':') == std::string::npos)
            continue;

        std::string full_path = base_path + name;
        char real_buf[PATH_MAX] = {};
        if (!realpath(full_path.c_str(), real_buf))
            continue;

        std::string real_path(real_buf);
        std::string uevent_path = real_path + "/uevent";

        DeviceType deviceType;
        std::string vid, pid;
        bool ok;
        std::tie(ok, vid, pid) = ParseVidPid(uevent_path);

        if (!ok || !MatchToExpectedVidPidPairs(vid, pid, deviceType))
            continue;

        std::string tty_path = real_path + "/tty";
        DIR* tty_dir = opendir(tty_path.c_str());
        if (!tty_dir)
            continue;

        while (dirent* tty_entry = readdir(tty_dir))
        {
            std::string tty_name = tty_entry->d_name;
            if (tty_name == "." || tty_name == "..")
                continue;

            std::string dev_path = "/dev/" + tty_name;
            struct stat s
            {
            };
            if (stat(dev_path.c_str(), &s) != 0 || !S_ISCHR(s.st_mode))
                continue;

            DeviceInfo info = {0};
            strncpy(info.serialPort, dev_path.c_str(), sizeof(info.serialPort) - 1);
            info.serialPort[sizeof(info.serialPort) - 1] = '\0';
            info.deviceType = DiscoverDeviceType(info.serialPort);

            if (info.deviceType != DeviceType::Unknown)
                devices.push_back(info);
            else
                LOG_WARNING(LOG_TAG, "Failed to detect device type for port: %s", info.serialPort);
        }

        closedir(tty_dir);
    }

    closedir(dir);
    return devices;
}

} // namespace RealSenseID
