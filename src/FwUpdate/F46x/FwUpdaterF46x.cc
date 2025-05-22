// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/DeviceController.h"
#include "FwUpdaterF46x.h"
#include "PacketManager/Timer.h"
#include "FwUpdateEngineF46x.h"
#include "Logger.h"
#include "Utilities.h"

#include <algorithm>
#include <fstream>
#include <exception>
#include <regex>
#include <sstream>

namespace RealSenseID
{

namespace FwUpdateF46x
{

static const char* LOG_TAG = "FwUpdaterF46x";
static constexpr long NORMAL_BAUD_RATE = 115200;
static const char* MODULE_OPFW = "OPFW";
static const char* MODULE_RECOG = "RECOG";

static bool DoesFileExist(const char* path)
{
    std::ifstream f(path);
    return f.good();
}

bool FwUpdaterF46x::ExtractFwInformation(const char* binPath, std::string& outFwVersion, std::string& outRecognitionVersion,
                                         std::vector<std::string>& moduleNames) const
{
    try
    {
        outFwVersion.clear();
        outRecognitionVersion.clear();
        moduleNames.clear();

        if (!DoesFileExist(binPath))
        {
            return false;
        }

        FwUpdateEngineF46x update_engine;
        auto modules = update_engine.ModulesFromFile(binPath);

        for (const auto& module : modules)
        {
            moduleNames.push_back(module.name);
            if (module.name == MODULE_OPFW)
            {
                outFwVersion = module.version;
            }
            else if (module.name == MODULE_RECOG)
            {
                outRecognitionVersion = module.version;
            }
        }

        if (outFwVersion.empty() || outRecognitionVersion.empty())
        {
            return false;
        }

        return true;
    }
    catch (const std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return false;
    }
}

Status FwUpdaterF46x::UpdateModules(FwUpdater::EventHandler* handler, FwUpdater::Settings settings, const char* binPath) const
{
    LOG_DEBUG(LOG_TAG, "UpdateModules Entry");
    try
    {
        // Check firmware upgrade file exists.
        if (!DoesFileExist(binPath))
        {
            LOG_ERROR(LOG_TAG, "file does not exist: :%s", binPath);
            return Status::Error;
        }

        auto callback_wrapper = [&handler](float progress) {
            if (handler != nullptr)
            {
                handler->OnProgress(progress);
            }
        };

        FwUpdateEngineF46x::Settings internal_settings;
        internal_settings.fw_filename = binPath;
        internal_settings.baud_rate = NORMAL_BAUD_RATE;
        internal_settings.serial_config = settings.serial_config;
        internal_settings.force_full = settings.force_full;

        FwUpdateEngineF46x update_engine;
        auto modules = update_engine.ModulesFromFile(binPath);
        PacketManager::Timer timer;
        update_engine.BurnModules(internal_settings, modules, callback_wrapper);
        auto elapsed_seconds = timer.Elapsed().count() / 1000;
        LOG_INFO(LOG_TAG, "Firmware update success (duration %lldm:%llds)", elapsed_seconds / 60, elapsed_seconds % 60);
        return Status::Ok;
    }
    catch (const std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return Status::Error;
    }
}

struct FirmwareVersion
{
public:
    int fwMajor, fwMinor;
    FirmwareVersion(const int& fwMajor, const int& fwMinor) : fwMajor(fwMajor), fwMinor(fwMinor)
    {
    }
    bool operator<(const FirmwareVersion& other) const
    {
        // We want to switch to OPFW_FIRST only once the major version number passes the one from the critical version.
        // Therefore we ignore the minor version number in the comparison.
        // return fwMajor < other.fwMajor || (fwMajor == other.fwMajor && fwMinor < other.fwMinor);
        return fwMajor < other.fwMajor;
    }

    bool operator>(const FirmwareVersion& other) const
    {
        return other < *this;
    }

    bool operator<=(const FirmwareVersion& other) const
    {
        return !(*this > other);
    }

    bool operator>=(const FirmwareVersion& other) const
    {
        return !(*this < other);
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << fwMajor << "." << fwMinor << ".#.#";
        return ss.str();
    }
};

} // namespace FwUpdateF46x
} // namespace RealSenseID
