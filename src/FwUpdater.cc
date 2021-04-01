// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#include "RealSenseID/FwUpdater.h"
#include "FwUpdate/FwUpdateEngine.h"
#include "PacketManager/Timer.h"
#include "Logger.h"

#include <fstream>
#include <exception>

namespace RealSenseID
{
using namespace FwUpdate;

static const char* LOG_TAG = "FwUpdater";

static constexpr long NORMAL_BAUD_RATE = 115200;
static constexpr long FAST_BAUD_RATE = 230400;
static constexpr long FASTER_BAUD_RATE = 460800;

static bool DoesFileExist(const char* path)
{
    std::ifstream f(path);
    return f.good();
}

bool FwUpdater::ExtractFwVersion(const char* binPath, std::string& outFwVersion, std::string& outRecognitionVersion) const
{
    try
    {
        outFwVersion.clear();
        outRecognitionVersion.clear();

        if (!DoesFileExist(binPath))
        {
            return false;
        }

        FwUpdateEngine update_engine;
        auto modules = update_engine.ModulesFromFile(binPath);

        for (const auto& module : modules)
        {
            if (module.name == "OPFW")
            {
                outFwVersion = module.version;
            }
            else if (module.name == "RECOG")
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

Status FwUpdater::Update(FwUpdater::EventHandler* handler, Settings settings, const char* binPath) const
{
    try
    {
        // Check firmware upgrade file exists.
        if (!DoesFileExist(binPath))
        {
            LOG_ERROR(LOG_TAG, "file does not exist: :%s", binPath);
            return Status::Error;
        }

        auto callback_wrapper = [&handler](float progress) {
            LOG_INFO(LOG_TAG, "Progress: %d%%", static_cast<int>(progress * 100));
            if (handler != nullptr)
            {
                handler->OnProgress(progress);
            }
        };

        FwUpdateEngine::Settings internal_settings;
        internal_settings.fw_filename = binPath;
        internal_settings.baud_rate = NORMAL_BAUD_RATE;
        internal_settings.port = settings.port;
        internal_settings.force_full = settings.force_full;
#ifdef ANDROID
        internal_settings.android_config = settings.android_config;
#endif

        FwUpdateEngine update_engine;
        auto modules = update_engine.ModulesFromFile(binPath);
        PacketManager::Timer timer;
        update_engine.BurnModules(internal_settings, modules, callback_wrapper);
        auto elapsed_seconds = timer.Elapsed() / 1000;
        LOG_INFO(LOG_TAG, "Firmware update success (duration %lldm:%llds)", elapsed_seconds / 60, elapsed_seconds % 60);
        return Status::Ok;
    }
    catch (const std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return Status::Error;
    }
}
} // namespace RealSenseID
