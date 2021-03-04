// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#include "RealSenseID/FwUpdater.h"
#include "RealSenseID/DeviceController.h"
#include "RealSenseID/DiscoverDevices.h"
#include "RealSenseID/Version.h"
#include <iostream>
#include <sstream>
#include <memory>
#include <exception>
#include <cstring>
#include <utility>
#include <vector>


static constexpr int SUCCESS_MAIN = 0;
static constexpr int FAILURE_MAIN = 1;

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
    for (int i = 0; i < devices.size(); ++i)
    {
        const auto& device = devices.at(i);
        std::cout << " " << i + 1 << ") S/N: " << device.metadata->serial_number << " "
                  << "FW: " << device.metadata->fw_version << " "
                  << "Port: " << device.config->serialPort << " ("
                  << (device.config->serialPortType == RealSenseID::SerialType::USB ? "USB" : "UART") << ")\n";
    }

    int device_index = -1;

    while (device_index < 1 || device_index > devices.size())
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

static std::string ParseFirmwareVersion(const std::string& full_version)
{
    std::stringstream version_stream(full_version);
    std::string section;
    while (std::getline(version_stream, section, '|'))
    {
        if (section.find("OPFW:") != section.npos)
        {
            auto pos = section.find(":");
            auto sub = section.substr(pos + 1, section.npos);
            return sub;
        }
    }

    return "Unknown";
}

/* Command line arguments */

struct CommandLineArgs
{
    bool is_valid = false;        // was parsing successful
    bool force_version = false;   // force non-compatible versions
    bool force_full = false;      // force update of all modules even if already exist in the fw
    bool is_interactive = false;  // ask user for approval
    std::string fw_file = "";     // path to firmware update binary
    std::string serial_port = ""; // serial port
    RealSenseID::SerialType serial_type = RealSenseID::SerialType::USB; // serial type
};

static CommandLineArgs ParseCommandLineArgs(int argc, char* argv[])
{
    CommandLineArgs args;

    if (argc < 2)
    {
        std::cout << "usage: " << argv[0]
                  << " --file <bin path> [--port <COM#>] [{--uart, --usb}] [--force-version] [--force-full] [--interactive]\n";
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
        else if (strcmp(argv[i], "--usb") == 0)
        {
            args.serial_type = RealSenseID::SerialType::USB;
        }
        else if (strcmp(argv[i], "--uart") == 0)
        {
            args.serial_type = RealSenseID::SerialType::UART;
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
    }

    // Make sure all required options are available.
    if (!args.fw_file.empty())
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
        metadata.fw_version = ParseFirmwareVersion(fw_version);

    std::string serial_number;
    device_controller.QuerySerialNumber(serial_number);
    if (!serial_number.empty())
        metadata.serial_number = serial_number;

    device_controller.Disconnect();

    return metadata;
}

struct FwUpdaterCliEventHandler : public RealSenseID::FwUpdater::EventHandler
{
    virtual void OnProgress(float progress) override
    {
        constexpr int progress_bars = 80;
        std::cout << "[";
        int progress_marker = static_cast<int>(progress_bars * progress);
        for (int bar = 0; bar < progress_bars; ++bar)
        {
            char to_print = bar < progress_marker ? ':' : ' ';
            std::cout << to_print;
        }
        std::cout << "] " << static_cast<int>(progress * 100) << " %\r";
        std::cout.flush();
    }
};

int main(int argc, char* argv[])
{
    // parse cli args
    auto args = ParseCommandLineArgs(argc, argv);
    if (!args.is_valid)
        return FAILURE_MAIN;


    // populate device list
    std::vector<FullDeviceInfo> devices_info;
    bool auto_detect = args.serial_port.empty();
    if (auto_detect)
    {
        std::cout << "Using device auto detection...\n\n";

        auto detected_devices = RealSenseID::DiscoverDevices();

        for (const auto& detected_device : detected_devices)
        {
            auto metadata = QueryDeviceMetadata(
                RealSenseID::SerialConfig {detected_device.serialPortType, detected_device.serialPort});

            FullDeviceInfo device {std::make_unique<DeviceMetadata>(metadata),
                                   std::make_unique<RealSenseID::DeviceInfo>(detected_device)};

            devices_info.push_back(std::move(device));
        }
    }
    else
    {
        std::cout << "Using manual device selection...\n\n";

        auto metadata = QueryDeviceMetadata(RealSenseID::SerialConfig {args.serial_type, args.serial_port.c_str()});

        auto device_info = std::make_unique<RealSenseID::DeviceInfo>();
        ::strncpy(device_info->serialPort, args.serial_port.data(), args.serial_port.size());
        device_info->serialPortType = args.serial_type;

        FullDeviceInfo device {std::make_unique<DeviceMetadata>(metadata), std::move(device_info)};

        devices_info.push_back(std::move(device));
    }

    if (devices_info.empty())
    {
        std::cout << "No devices found!\n";
        return FAILURE_MAIN;
    }

    // if more than one device exists - ask user to select
    auto id = devices_info.size() == 1 ? 0 : UserDeviceSelection(devices_info);
    const auto& selected_device = devices_info.at(id);

    // extract fw version from update file
    RealSenseID::FwUpdater fw_updater;

    const auto& bin_path = args.fw_file.c_str();

    std::string new_fw_version;
    auto is_valid = fw_updater.ExtractFwVersion(bin_path, new_fw_version);

    if (!is_valid)
    {
        std::cout << "Invalid firmware file !\n";
        return FAILURE_MAIN;
    }

    // check compatibility with host -
    // 1. vs current fw version
    // 2. vs new fw version
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
    std::cout << " * Firmware update path: " << current_fw_version << " -> " << new_fw_version << "\n";
    std::cout << "\n";

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
        std::cout << "Version is incompatible with the current host version.\n";
        return FAILURE_MAIN;
    }

    // create fw-updater settings and progress callback
    auto event_handler = std::make_unique<FwUpdaterCliEventHandler>();
    RealSenseID::FwUpdater::Settings settings;
    settings.port = selected_device.config->serialPort;
    settings.force_full = args.force_full;

    // attempt firmware update and return succcess/failure according to result
    auto success = fw_updater.Update(event_handler.get(), settings, args.fw_file.c_str()) == RealSenseID::Status::Ok;

    std::cout << "\n\n";
    std::cout << "Firmware update" << (success ? " finished successfully " : " failed ") << "\n";

    return success ? SUCCESS_MAIN : FAILURE_MAIN;
}
