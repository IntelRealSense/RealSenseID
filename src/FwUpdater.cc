// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/DeviceController.h"
#include "RealSenseID/FwUpdater.h"
#include "FwUpdate/FwUpdateEngine.h"
#include "PacketManager/Timer.h"
#include "Logger.h"
#include "FwUpdate/Utilities.h"

#include <algorithm>
#include <fstream>
#include <exception>
#include <regex>
#include <sstream>

namespace RealSenseID
{
using namespace FwUpdate;

static const char* LOG_TAG = "FwUpdater";

static constexpr long NORMAL_BAUD_RATE = 115200;
static constexpr long FAST_BAUD_RATE = 230400;
static constexpr long FASTER_BAUD_RATE = 460800;

static const char* MODULE_OPFW = "OPFW";
static const char* MODULE_RECOG = "RECOG";

static constexpr int CRITICAL_FW_MAJOR = 3;
static constexpr int CRITICAL_FW_MINOR = 1;

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

bool FwUpdater::IsEncryptionSupported(const char* binPath, const std::string& deviceSerialNumber) const
{
    uint8_t otpEncryptionVersion = RealSenseID::FwUpdate::ParseUfifToOtpEncryption(binPath);
    uint8_t expected_version = 0;
    const std::regex ver1_option1("12[02]\\d6228\\d{4}.*");
    const std::regex ver1_option2("\\d{4}6229\\d{4}.*");
    const std::regex ver1_option3("\\d{4}62[3-9]\\d{5}.*"); 
    if (std::regex_match(deviceSerialNumber, ver1_option1) ||
            std::regex_match(deviceSerialNumber, ver1_option2) ||
            std::regex_match(deviceSerialNumber, ver1_option3))
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

struct FirmwareVersion
{
    private:
        int fwMajor, fwMinor;

    public:
    FirmwareVersion(const int& fwMajor, const int& fwMinor) : fwMajor(fwMajor), fwMinor(fwMinor) {}
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

    bool operator<=(const FirmwareVersion& other)
    {
        return !(*this > other);
    }
    
    bool operator>=(const FirmwareVersion& other)
    {
        return !(*this < other);
    }

    std::string ToString()
    {
        std::stringstream ss;
        ss << fwMajor << "." << fwMinor << ".#.#";
        return ss.str();
    }
};

static std::string ExtractModuleFromVersion(const std::string& module_name, const std::string& full_version)
{
    std::stringstream version_stream(full_version);
    std::string section;
    while (std::getline(version_stream, section, '|'))
    {
        if (section.find(module_name) != section.npos)
        {
            auto pos = section.find(":");
            auto sub = section.substr(pos + 1, section.npos);
            return sub;
        }
    }

    return "Unknown";
}

static std::string ParseFirmwareVersion(const std::string& full_version)
{
    return ExtractModuleFromVersion("OPFW:", full_version);
}

static FirmwareVersion StringToFirmwareVersion(std::string outFwVersion)
{
    const std::regex r("(\\d+)\\.(\\d+)\\.\\d+\\.\\d+");
    std::smatch base_match;
    if (std::regex_match(outFwVersion, base_match, r))
    {
        int major = atoi(base_match[1].str().c_str());
        int minor = atoi(base_match[2].str().c_str());
        return FirmwareVersion(major, minor);
    }
    throw std::runtime_error("Unknown firmware version structure");
}

static FirmwareVersion QueryDeviceFirmwareVersion(const FwUpdater::Settings& settings)
{
    std::string firmwareVersion = "";
    RealSenseID::DeviceController device_controller;
    Status s;
#ifdef ANDROID
    s = device_controller.Connect(settings.android_config);
#else
    s = device_controller.Connect(SerialConfig({settings.port}));
#endif
    if (s != Status::Ok)
    {
        throw std::runtime_error("Failed to connect to device");
    }
    std::string allVersions;
    device_controller.QueryFirmwareVersion(allVersions);
    if (!allVersions.empty())
    {
        firmwareVersion = ParseFirmwareVersion(allVersions);
    }
    device_controller.Disconnect();
    if (firmwareVersion.empty())
    {
        throw std::runtime_error("Failed to extract device firmware version");
    }
    return StringToFirmwareVersion(firmwareVersion);
}

static FirmwareVersion RetrieveBinFileFirmwareVersion(const char* binPath)
{
    if (!DoesFileExist(binPath))
    {
        throw std::runtime_error("Binary file not found");
    }
    FwUpdateEngine update_engine;
    auto modules = update_engine.ModulesFromFile(binPath);

    std::vector<ModuleInfo>::iterator it =
        std::find_if(modules.begin(), modules.end(), [](const auto& module) { return module.name == MODULE_OPFW; });
    if (it == modules.end())
    {
        throw std::runtime_error("OPFW module not found in binary.");
    }
    std::string outFwVersion = it->version;
    return StringToFirmwareVersion(outFwVersion); 
}

FwUpdater::UpdatePolicyInfo FwUpdater::DecideUpdatePolicy(const Settings& settings, const char* binPath) const
{
    FirmwareVersion criticalFwVer(CRITICAL_FW_MAJOR, CRITICAL_FW_MINOR);
    try
    {
        FirmwareVersion deviceFwVer = QueryDeviceFirmwareVersion(settings);
        FirmwareVersion binFileFwVer = RetrieveBinFileFirmwareVersion(binPath);
        UpdatePolicyInfo upi;
        if (binFileFwVer <= criticalFwVer)
        {
            upi.policy = UpdatePolicyInfo::UpdatePolicy::CONTINOUS;
        }
        else if (deviceFwVer >= criticalFwVer)
        {
            // binFileFwVer > criticalFwVer && deviceFwVer >= criticalFwVer
            upi.policy = UpdatePolicyInfo::UpdatePolicy::OPFW_FIRST;
        }
        else
        {
            // deviceFwVer < criticalFwVer < binFileFwVer
            upi.policy = UpdatePolicyInfo::UpdatePolicy::REQUIRE_INTERMEDIATE_FW;
            upi.intermediate = criticalFwVer.ToString();
        }
        return upi;
    }
    catch (const std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return UpdatePolicyInfo({UpdatePolicyInfo::UpdatePolicy::NOT_ALLOWED, ""});
    }
}
} // namespace RealSenseID
