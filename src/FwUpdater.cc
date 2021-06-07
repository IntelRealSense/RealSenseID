// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#include "RealSenseID/FwUpdater.h"
#include "FwUpdate/FwUpdateEngine.h"
#include "PacketManager/Timer.h"
#include "Logger.h"
#include "FwUpdate/Utilities.h"

#include <algorithm>
#include <fstream>
#include <exception>
#include <regex>

namespace RealSenseID
{
using namespace FwUpdate;

static const char* LOG_TAG = "FwUpdater";

static constexpr long NORMAL_BAUD_RATE = 115200;
static constexpr long FAST_BAUD_RATE = 230400;
static constexpr long FASTER_BAUD_RATE = 460800;

static const char* MODULE_OPFW = "OPFW";
static const char* MODULE_RECOG = "RECOG";

static bool DoesFileExist(const char* path)
{
    std::ifstream f(path);
    return f.good();
}

bool FwUpdater::ExtractFwInformation(const char* binPath, std::string& outFwVersion,
                                 std::string& outRecognitionVersion,
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

        FwUpdateEngine update_engine;
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

bool FwUpdater::IsEncryptionSupported(const char* binPath, const std::string& deviceSerialNumber)
{
    uint8_t otpEncryptionVersion = RealSenseID::FwUpdate::ParseUfifToOtpEncryption(binPath);

    uint8_t expected_version = 0;
    
    const std::regex ver1_option1("12[02]\\d6228\\d{4}.*");
    const std::regex ver1_option2("\\d{4}6229\\d{4}.*");
    if (std::regex_match(deviceSerialNumber, ver1_option1) || std::regex_match(deviceSerialNumber, ver1_option2))
        expected_version = 1;

    return expected_version == otpEncryptionVersion;
}

Status FwUpdater::UpdateModules(EventHandler* handler, Settings settings, const char* binPath,
                              const std::vector<std::string>& moduleNames) const
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

        modules.erase(std::remove_if(modules.begin(), modules.end(),
                                         [moduleNames](const ModuleInfo& mod_info) { 
                    for (auto moduleName: moduleNames)
                    {
                        if (mod_info.name.compare(moduleName) == 0)
                            return false;
                    }
                    return true;
                }), modules.end());
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
