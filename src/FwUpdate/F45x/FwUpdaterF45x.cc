// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/DeviceController.h"
#include "RealSenseID/FwUpdater.h"
#include "RealSenseID/SerialConfig.h"
#include "FwUpdateEngineF45x.h"
#include "FwUpdaterF45x.h"
#include "PacketManager/Timer.h"
#include "Logger.h"
#include "Utilities.h"

#include <algorithm>
#include <fstream>
#include <exception>
#include <regex>
#include <sstream>

namespace RealSenseID
{

namespace FwUpdateF45x
{

static const char* LOG_TAG = "FwUpdateF45x";

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

bool FwUpdaterF45x::ExtractFwInformation(const char* binPath, std::string& outFwVersion, std::string& outRecognitionVersion,
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

        FwUpdateF45x::FwUpdateEngineF45x update_engine;
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

bool FwUpdaterF45x::IsSkuCompatible(const FwUpdater::Settings& settings, const char* binPath, int& expectedSkuVer, int& deviceSkuVer) const
{
    expectedSkuVer = -1;
    deviceSkuVer = -1;
    try
    {
        uint8_t binOtpEncVer = ParseUfifToOtpEncryption(binPath);
        expectedSkuVer = static_cast<int>(binOtpEncVer) + 1;
        uint8_t deviceOtpEncVer = 0;
        RealSenseID::DeviceController device_controller(DeviceType::F45x);
        Status s = device_controller.Connect(settings.serial_config);
        if (s != Status::Ok)
        {
            throw std::runtime_error("Failed to connect to device");
        }
        s = device_controller.QueryOtpVersion(deviceOtpEncVer);
        if (s == Status::Ok)
        {
            deviceSkuVer = (deviceOtpEncVer - '0') + 1;
            LOG_INFO(LOG_TAG, "QueryOtpVersion: SKU %d", deviceSkuVer);
        }
        else
        {
            // older fw versions do not support querying otp version. Use serial number to decide
            LOG_INFO(LOG_TAG, "Device does not support QueryOtpVersion. Quering SN");
            std::string sn;
            if (device_controller.QuerySerialNumber(sn) != Status::Ok)
            {
                LOG_INFO(LOG_TAG, "Failed getting serial number. Assuming SKU compatible");
                return true;
            }
            /*
             * Examples of SKU2
             * 120X6228XXXXXXXXXXXXXXXX-XXX
             * 122X6228XXXXXXXXXXXXXXXX-XXX
             * XXXX6229XXXXXXXXXXXXXXXX-XXX
             */
            const std::regex reg1("12[02]\\d6228\\d{4}.*");
            const std::regex reg2("\\d{4}6229\\d{4}.*");
            const std::regex reg3("\\d{4}62[3-9]\\d{5}.*");
            if (std::regex_match(sn, reg1) || std::regex_match(sn, reg2) || std::regex_match(sn, reg3))
                deviceSkuVer = 2;
            else
                deviceSkuVer = 1;

            LOG_INFO(LOG_TAG, "SN to SKU: %s -> SKU %d", sn.c_str(), deviceSkuVer);
        }

        return expectedSkuVer == deviceSkuVer;
    }
    catch (const std::exception& ex)
    {
        LOG_EXCEPTION(LOG_TAG, ex);
        return false;
    }
}

Status FwUpdaterF45x::UpdateModules(FwUpdater::EventHandler* handler, FwUpdater::Settings settings, const char* binPath) const
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

        FwUpdateEngineF45x::Settings internal_settings;
        internal_settings.fw_filename = binPath;
        internal_settings.baud_rate = NORMAL_BAUD_RATE;
        internal_settings.serial_config = settings.serial_config;
        internal_settings.force_full = settings.force_full;

        FwUpdateEngineF45x update_engine;
        auto modules = update_engine.ModulesFromFile(binPath);
        PacketManager::Timer timer;
        update_engine.BurnModules(internal_settings, modules, callback_wrapper);
        const auto elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(timer.Elapsed()).count();
        const auto minutes = elapsed_seconds / 60;
        const auto seconds = elapsed_seconds % 60;
        LOG_INFO(LOG_TAG, "Firmware update success (duration %lldm:%llds)", minutes, seconds);
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
} // namespace FwUpdateF45x
} // namespace RealSenseID
