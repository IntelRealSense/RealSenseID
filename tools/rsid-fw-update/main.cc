// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#include "RealSenseID/FwUpdater.h"
#include "RealSenseID/DeviceController.h"
#include "RealSenseID/DiscoverDevices.h"
#include "RealSenseID/Version.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <memory>
#include <exception>
#include <cstring>
#include <utility>
#include <vector>
#include <thread>
#include <chrono>


static constexpr int SUCCESS_MAIN = 0;
static constexpr int FAILURE_MAIN = 1;
static constexpr int MIN_WAIT_FOR_DEVICE = 6;
static constexpr int MAX_WAIT_FOR_DEVICE = 30;
static const std::string OPFW = "OPFW";
static const std::string RECOG = "RECOG";

struct DeviceMetadata
{
    std::string serial_number = "Unknown";
    std::string fw_version = "Unknown";
};

struct FullDeviceInfo
{
    std::unique_ptr<DeviceMetadata> metadata;
    std::unique_ptr<RealSenseID::DeviceInfo> config;
};

/* User interaction */
static int UserDeviceSelection(const std::vector<FullDeviceInfo>& devices)
{
    std::cout << "Detected devices:\n";
    for (size_t i = 0; i < devices.size(); ++i)
    {
        const auto& device = devices.at(i);
        std::cout << " " << i + 1 << ") S/N: " << device.metadata->serial_number << " "
                  << "FW: " << device.metadata->fw_version << " "
                  << "Port: " << device.config->serialPort << "\n";
    }

    int device_index = -1;

    while (device_index < 1 || device_index > static_cast<int>(devices.size()))
    {
        std::cout << "> ";

        std::string line;
        std::getline(std::cin, line);

        try
        {
            device_index = std::stoi(line);
        }
        catch (...)
        {
            device_index = -1;
        }
    }

    std::cout << "\n";

    return device_index - 1;
}

static bool UserApproval()
{
    char key = '0';
    while (key != 'y' && key != 'n')
    {
        std::cout << "> ";
        std::cin >> key;
    }

    return key == 'y';
}

/* Misc */

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


/* Command line arguments */

struct CommandLineArgs
{
    bool is_valid = false;        // was parsing successful
    bool force_version = false;   // force non-compatible versions
    bool force_full = false;      // force update of all modules even if already exist in the fw
    bool is_interactive = false;  // ask user for confirmation before starting
    bool auto_approve = false;    // automatically approve all (use default params)
    std::string fw_file = "";     // path to firmware update binary
    std::string serial_port = ""; // serial port
};

static CommandLineArgs ParseCommandLineArgs(int argc, char* argv[])
{
    CommandLineArgs args;

    if (argc < 2)
    {
        std::cout << "usage: " << argv[0]
                  << " --file <bin path> [--port <COM#>] [--force-version] [--force-full] [--interactive / --auto-approve]\n";
        return args;
    }

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--file") == 0)
        {
            if (i + 1 < argc)
                args.fw_file = argv[++i];
        }
        else if (strcmp(argv[i], "--port") == 0)
        {
            if (i + 1 < argc)
                args.serial_port = argv[++i];
        }
        else if (strcmp(argv[i], "--force-full") == 0)
        {
            args.force_full = true;
        }
        else if (strcmp(argv[i], "--force-version") == 0)
        {
            args.force_version = true;
        }
        else if (strcmp(argv[i], "--interactive") == 0)
        {
            args.is_interactive = true;
        }
        else if (strcmp(argv[i], "--auto-approve") == 0)
        {
            args.auto_approve = true;
        }
    }

    // Make sure all required options are available.
    if (args.fw_file.empty())
        return args;

    if (args.is_interactive && args.auto_approve)
    {
        std::cout << "--is-interactive and --auto-approve flags do not co-exist. Choose either or none." << std::endl;
        return args;
    }
    args.is_valid = true;
    return args;
}

static DeviceMetadata QueryDeviceMetadata(const RealSenseID::SerialConfig& serial_config)
{
    DeviceMetadata metadata;

    RealSenseID::DeviceController device_controller;

    device_controller.Connect(serial_config);

    std::string fw_version;
    device_controller.QueryFirmwareVersion(fw_version);
    if (!fw_version.empty())
    {
        metadata.fw_version = ParseFirmwareVersion(fw_version);
    }

    std::string serial_number;
    device_controller.QuerySerialNumber(serial_number);
    if (!serial_number.empty())
        metadata.serial_number = serial_number;

    device_controller.Disconnect();

    return metadata;
}


static bool DetectDevices(std::vector<FullDeviceInfo>& out_devices_info)
{
    std::cout << "Using device auto detection...\n";
    out_devices_info.clear();
    auto detected_devices = RealSenseID::DiscoverDevices();
    for (const auto& detected_device : detected_devices)
    {
        auto metadata = QueryDeviceMetadata(RealSenseID::SerialConfig {detected_device.serialPort});

        FullDeviceInfo device {std::make_unique<DeviceMetadata>(metadata), std::make_unique<RealSenseID::DeviceInfo>(detected_device)};

        out_devices_info.push_back(std::move(device));
    }
    if (out_devices_info.empty())
    {
        std::cout << "No devices found!\n";
        return false;
    }
    return true;
}


struct FwUpdaterCliEventHandler : public RealSenseID::FwUpdater::EventHandler
{
public:
    FwUpdaterCliEventHandler(float minValue, float maxValue) : m_minValue(minValue), m_maxValue(maxValue)
    {
    }

    virtual void OnProgress(float progress) override
    {
        float adjustedProgress = m_minValue + progress * (m_maxValue - m_minValue);
        constexpr int progress_bars = 80;
        std::cout << "[";
        int progress_marker = static_cast<int>(progress_bars * adjustedProgress);
        for (int bar = 0; bar < progress_bars; ++bar)
        {
            char to_print = bar < progress_marker ? ':' : ' ';
            std::cout << to_print;
        }
        std::cout << "] " << static_cast<int>(adjustedProgress * 100) << " %\r";
        std::cout.flush();
    }

private:
    float m_minValue, m_maxValue;
};

int main(int argc, char* argv[])
{
    try
    {
        // parse cli args
        auto args = ParseCommandLineArgs(argc, argv);
        if (!args.is_valid)
            return FAILURE_MAIN;

        // populate device list

        FullDeviceInfo selected_device;
        bool auto_detect = args.serial_port.empty();
        if (auto_detect)
        {
            std::vector<FullDeviceInfo> devices_info;
            if (!DetectDevices(devices_info))
            {
                return FAILURE_MAIN;
            }
            // if more than one device exists - ask user to select
            auto id = devices_info.size() == 1 ? 0 : UserDeviceSelection(devices_info);
            selected_device.config = std::move(devices_info.at(id).config);
            selected_device.metadata = std::move(devices_info.at(id).metadata);
        }
        else
        {
            std::cout << "Using manual device selection...\n\n";

            auto metadata = QueryDeviceMetadata(RealSenseID::SerialConfig {args.serial_port.c_str()});

            auto device_info = std::make_unique<RealSenseID::DeviceInfo>();
            ::strncpy(device_info->serialPort, args.serial_port.data(), args.serial_port.size());

            selected_device = {std::make_unique<DeviceMetadata>(metadata), std::move(device_info)};
        }

        // extract fw version from update file
        RealSenseID::FwUpdater fw_updater;

        const auto& bin_path = args.fw_file.c_str();

        std::string new_fw_version;
        std::string new_recognition_version;
        std::vector<std::string> moduleNames;
        auto is_valid = fw_updater.ExtractFwInformation(bin_path, new_fw_version, new_recognition_version, moduleNames);

        if (!is_valid)
        {
            std::cout << "Invalid firmware file !\n";
            return FAILURE_MAIN;
        }

        RealSenseID::FwUpdater::Settings settings;
        settings.port = selected_device.config->serialPort;
        settings.force_full = args.force_full;

        int expectedSkuVer = 0, deviceSkuVer = 0;
        if (!fw_updater.IsSkuCompatible(settings, bin_path, expectedSkuVer, deviceSkuVer))
        {
            std::cout << "Device does not support the encryption applied on the firmware.\nReplace firmware binary to SKU" << deviceSkuVer
                      << "\n";
            return FAILURE_MAIN;
        }

        // check compatibility with host
        const auto& current_fw_version = selected_device.metadata->fw_version;
        const auto current_compatible = RealSenseID::IsFwCompatibleWithHost(current_fw_version);
        const auto new_compatible = RealSenseID::IsFwCompatibleWithHost(new_fw_version);


        // show summary to user - update path, compatibility checks
        std::cout << "\n";
        std::cout << "Summary:\n";
        std::cout << " * Serial number: " << selected_device.metadata->serial_number << "\n";
        std::cout << " * Serial port: " << selected_device.config->serialPort << "\n";
        std::cout << " * " << (current_compatible ? "Compatible" : "Incompatible") << " with current device firmware\n";
        std::cout << " * " << (new_compatible ? "Compatible" : "Incompatible") << " with new device firmware\n";
        std::cout << " * Firmware update path:\n";
        std::cout << "     * OPFW: " << current_fw_version << " -> " << new_fw_version << "\n";
        std::cout << "\n";

        auto updatePolicyInfo = fw_updater.DecideUpdatePolicy(settings, args.fw_file.c_str());
        if (updatePolicyInfo.policy == RealSenseID::FwUpdater::UpdatePolicyInfo::UpdatePolicy::NOT_ALLOWED)
        {
            // Currently never returned
            std::cout << "Update from current device firmware to selected firmware file is unsupported by this host "
                         "application."
                      << std::endl;
            return FAILURE_MAIN;
        }
        if (updatePolicyInfo.policy == RealSenseID::FwUpdater::UpdatePolicyInfo::UpdatePolicy::REQUIRE_INTERMEDIATE_FW)
        {
            std::cout << "Firmware cannot be updated directly to the chosen version.\n"
                      << "Flash firmware version " << updatePolicyInfo.intermediate << " first." << std::endl;
            return FAILURE_MAIN;
        }

        // ask user for approval if interactive
        if (args.is_interactive)
        {
            std::cout << "Proceed with update? (y/n)\n";
            bool user_agreed = UserApproval();
            if (!user_agreed)
                return FAILURE_MAIN;
            std::cout << "\n";
        }

        // allow bypass of incompatible version if forced
        if (!new_compatible && !args.force_version)
        {
            std::cout << "Version is incompatible with the current host version!\n";
            std::cout << "Use --force-version to force the update.\n ";
            return FAILURE_MAIN;
        }


        auto success = false;
        if (updatePolicyInfo.policy == RealSenseID::FwUpdater::UpdatePolicyInfo::UpdatePolicy::CONTINOUS)
        {
            std::unique_ptr<FwUpdaterCliEventHandler> event_handler = std::make_unique<FwUpdaterCliEventHandler>(0.f, 1.f);
            RealSenseID::Status status = fw_updater.UpdateModules(event_handler.get(), settings, args.fw_file.c_str(), moduleNames);
            success = status == RealSenseID::Status::Ok;
        }
        else if (updatePolicyInfo.policy == RealSenseID::FwUpdater::UpdatePolicyInfo::UpdatePolicy::OPFW_FIRST)
        {
            // make sure OPFW is first module
            moduleNames.erase(std::remove_if(moduleNames.begin(), moduleNames.end(),
                                             [](const std::string& moduleName) { return moduleName.compare(OPFW) == 0; }),
                              moduleNames.end());
            moduleNames.insert(moduleNames.begin(), OPFW);
            settings.port = selected_device.config->serialPort;
            auto event_handler = std::make_unique<FwUpdaterCliEventHandler>(0.f, 1.f);
            RealSenseID::Status status = fw_updater.UpdateModules(event_handler.get(), settings, args.fw_file.c_str(), moduleNames);
            success = status == RealSenseID::Status::Ok;
        }

        std::cout << "\n\n";
        std::cout << "Firmware update" << (success ? " finished successfully " : " failed ") << "\n";

        return success ? SUCCESS_MAIN : FAILURE_MAIN;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Exception occured: " << ex.what() << std::endl;
        return FAILURE_MAIN;
    }
}
