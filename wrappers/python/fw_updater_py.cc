// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2024 Intel Corporation. All Rights Reserved.

#include "RealSenseID/FwUpdater.h"
#include "RealSenseID/DeviceController.h"
#include "RealSenseID/DiscoverDevices.h"
#include "RealSenseID/SerialConfig.h"

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include "rsid_py.h"
#include "RealSenseID/Version.h"
#include <string>
#include <exception>

#include <sstream>
#include <iostream>

namespace py = pybind11;

// Internal

static const std::string OPFW = "OPFW";

struct DeviceMetadata
{
    std::string serial_number = "Unknown";
    std::string fw_version = "Unknown";
    std::string recognition_version = "Unknown";
};

struct FullDeviceInfo
{
    DeviceMetadata metadata;
    RealSenseID::DeviceInfo config;
};

// Exported to py bindings

class FirmwareBinInfo
{
public:
    std::string fw_version;
    std::string recognition_version;
    std::vector<std::string> module_names;
};

class InvalidFirmwareException : public std::runtime_error
{
public:
    explicit InvalidFirmwareException(const std::string& message) : runtime_error(message)
    {
    }
};

class SKUMismatchException : public std::runtime_error
{
public:
    explicit SKUMismatchException(const std::string& message) : runtime_error(message)
    {
    }
};

class IncompatibleHostException : public std::runtime_error
{
public:
    explicit IncompatibleHostException(const std::string& message) : runtime_error(message)
    {
    }
};

class FWUpdatePolicyException : public std::runtime_error
{
public:
    explicit FWUpdatePolicyException(const std::string& message) : runtime_error(message)
    {
    }
};

class FWUpdateException : public std::runtime_error
{
public:
    explicit FWUpdateException(const std::string& message) : runtime_error(message)
    {
    }
};


using UpdateProgressClbkFun = std::function<void(float progress)>;

struct FwUpdaterEventHandler : public RealSenseID::FwUpdater::EventHandler, public std::enable_shared_from_this<FwUpdaterEventHandler>
{
public:
    explicit FwUpdaterEventHandler(UpdateProgressClbkFun& progressCallbackFn) : _progress_clbk {progressCallbackFn}
    {
    }

    void OnProgress(float progress) override
    {
        if (_progress_clbk)
        {
            _progress_clbk(progress);
        }
    }

private:
    UpdateProgressClbkFun& _progress_clbk;
};


class FWUpdaterPy
{
public:
    explicit FWUpdaterPy(const std::string& file_path, const std::string& port)
    {
        _updater = std::make_unique<RealSenseID::FwUpdater>();
        _file_path = file_path;
        _port = port;
    };

    FirmwareBinInfo GetFirmwareBinInfo()
    {
        FirmwareBinInfo bin_info;
        auto is_valid =
            _updater->ExtractFwInformation(_file_path.c_str(), bin_info.fw_version, bin_info.recognition_version, bin_info.module_names);
        if (!is_valid)
        {
            throw InvalidFirmwareException("Invalid firmware file: The specified file does not appear "
                                           "to be a valid firmware file");
        }
        _bin_info = std::make_unique<FirmwareBinInfo>(bin_info);
        return *_bin_info;
    };

    DeviceMetadata GetDeviceFirmwareInfo()
    {
        QueryDeviceInfo(true);
        return {_device_info->metadata};
    }

    std::tuple<bool, std::string> IsSkuCompatible()
    {
        RealSenseID::FwUpdater::Settings settings;
        settings.port = _port.c_str();

        std::stringstream message;
        int expectedSkuVer = 0, deviceSkuVer = 0;
        if (!_updater->IsSkuCompatible(settings, _file_path.c_str(), expectedSkuVer, deviceSkuVer))
        {
            message << "SKU/Firmware mismatch. Device does not support the encryption applied on the firmware. "
                    << "Replace firmware binary to SKU" << deviceSkuVer;
            return std::make_tuple(false, message.str());
        }
        return std::make_tuple(true, "Firmware file matches device SKU.");
    };

    std::tuple<bool, std::string> IsHostCompatible()
    {
        auto fw_info_ok = QueryFirmwareFileInfo(false);
        if (!std::get<0>(fw_info_ok))
        {
            return fw_info_ok;
        }

        // check compatibility with host
        const auto new_compatible = RealSenseID::IsFwCompatibleWithHost(_bin_info->fw_version);

        if (new_compatible)
        {
            return std::make_tuple(true, "Current host SDK is compatible with this firmware file.");
        }
        else
        {
            return std::make_tuple(false, "New firmware is not compatible with current host SDK version. "
                                          "Upgrading host SDK will be required. ");
        }
    }

    std::tuple<bool, std::string> IsPolicyCompatible()
    {
        auto fw_info_ok = QueryFirmwareFileInfo(false);
        if (!std::get<0>(fw_info_ok))
        {
            return fw_info_ok;
        }
        RealSenseID::FwUpdater::Settings settings;
        settings.port = _port.c_str();

        std::stringstream message;
        RealSenseID::FwUpdater::UpdatePolicyInfo updatePolicyInfo;
        {
            updatePolicyInfo = _updater->DecideUpdatePolicy(settings, _file_path.c_str());
        }
        switch (updatePolicyInfo.policy)
        {
        case RealSenseID::FwUpdater::UpdatePolicyInfo::UpdatePolicy::NOT_ALLOWED:
            return std::make_tuple(false, "Update from current device firmware to selected firmware file is "
                                          "unsupported by this host application.");
        case RealSenseID::FwUpdater::UpdatePolicyInfo::UpdatePolicy::REQUIRE_INTERMEDIATE_FW:
            message << "Firmware cannot be updated directly to the chosen version."
                    << "Flash firmware version " << updatePolicyInfo.intermediate << " first.";
            return std::make_tuple(false, message.str());
        default:
            break;
        }

        return std::make_tuple(true, "Upgrade policy is compatible with this firmware file.");
    };

    RealSenseID::Status Update(bool force_version = false, bool force_full = false, UpdateProgressClbkFun* progress_clbk_fun = nullptr)
    {
        QueryFirmwareFileInfo(true);
        QueryDeviceInfo(true);

        // Check for SKU match
        auto sku_compat = IsSkuCompatible();
        if (!std::get<0>(sku_compat))
        {
            throw SKUMismatchException(std::get<1>(sku_compat));
        }

        // Check for update policy
        auto policy_compat = IsPolicyCompatible();
        if (!std::get<0>(policy_compat))
        {
            throw FWUpdatePolicyException(std::get<1>(policy_compat));
        }

        // Check for host compatibility
        auto host_compat = IsHostCompatible();
        if (!std::get<0>(host_compat))
        {
            if (!force_version)
                throw IncompatibleHostException(std::get<1>(host_compat) + " force_version argument is required to override.");
        }

        // All clear - Perform actual update
        RealSenseID::FwUpdater::Settings settings;
        settings.port = _port.c_str();
        settings.force_full = force_full;

        RealSenseID::FwUpdater::UpdatePolicyInfo updatePolicyInfo;
        {
            updatePolicyInfo = _updater->DecideUpdatePolicy(settings, _file_path.c_str());
        }

        auto module_names(_bin_info->module_names);

        if (updatePolicyInfo.policy == RealSenseID::FwUpdater::UpdatePolicyInfo::UpdatePolicy::CONTINOUS)
        {
            _event_handler = std::make_shared<FwUpdaterEventHandler>(*progress_clbk_fun);
            return (_updater->UpdateModules(_event_handler.get(), settings, _file_path.c_str(), module_names));
        }
        else if (updatePolicyInfo.policy == RealSenseID::FwUpdater::UpdatePolicyInfo::UpdatePolicy::OPFW_FIRST)
        {
            // make sure OPFW is first module
            module_names.erase(std::remove_if(module_names.begin(), module_names.end(),
                                              [](const std::string& module_name) { return module_name == OPFW; }),
                               module_names.end());
            module_names.insert(module_names.begin(), OPFW);
            settings.port = _port.c_str();
            _event_handler = std::make_shared<FwUpdaterEventHandler>(*progress_clbk_fun);
            return (_updater->UpdateModules(_event_handler.get(), settings, _file_path.c_str(), module_names));
        }
        else
        {
            std::stringstream message;
            message << "Firmware cannot be updated due to policy exception. Policy value: " << static_cast<int>(updatePolicyInfo.policy)
                    << " is not handled in this SDK release.";
            throw FWUpdatePolicyException(message.str());
        }
    }

    void exit(py::handle type, py::handle value, py::handle traceback)
    {
        (void)type;
        (void)value;
        (void)traceback; // silence unused warnings
        _updater.reset();
        _bin_info.reset();
        _device_info.reset();
        _event_handler.reset();
    }

    ~FWUpdaterPy() = default;

private:
    std::unique_ptr<RealSenseID::FwUpdater> _updater;
    std::unique_ptr<FirmwareBinInfo> _bin_info;
    std::string _file_path;
    std::string _port;
    std::unique_ptr<FullDeviceInfo> _device_info;
    std::shared_ptr<FwUpdaterEventHandler> _event_handler;

    std::tuple<bool, std::string> QueryFirmwareFileInfo(bool raise_on_error)
    {
        try
        {
            GetFirmwareBinInfo();
            return std::make_tuple(true, "OK");
        }
        catch (std::exception& e)
        {
            if (raise_on_error)
            {
                throw InvalidFirmwareException(e.what());
            }
            return std::make_tuple(false, e.what());
        }
    }

    std::tuple<bool, std::string> QueryDeviceInfo(bool raise_on_error)
    {
        try
        {
            if (_device_info == nullptr)
            {
                auto metadata = QueryDeviceMetadata(RealSenseID::SerialConfig {_port.c_str()});
                RealSenseID::DeviceInfo device_info {};
                ::strncpy(device_info.serialPort, _port.data(), _port.size());
                FullDeviceInfo full_device_info {metadata, device_info};
                _device_info = std::make_unique<FullDeviceInfo>(full_device_info);
            }
            return std::make_tuple(true, "OK");
        }
        catch (std::exception& e)
        {
            if (raise_on_error)
            {
                throw e;
            }
            return std::make_tuple(false, e.what());
        }
    }

    DeviceMetadata QueryDeviceMetadata(const RealSenseID::SerialConfig& serial_config)
    {
        DeviceMetadata metadata;
        std::string fw_version;

        RealSenseID::DeviceController device_controller;
        device_controller.Connect(serial_config);
        device_controller.QueryFirmwareVersion(fw_version);
        device_controller.QuerySerialNumber(metadata.serial_number);
        device_controller.Disconnect();

        if (!fw_version.empty())
        {
            metadata.fw_version = ExtractModuleFromVersion("OPFW:", fw_version);
            metadata.recognition_version = ExtractModuleFromVersion("RECOG:", fw_version);
        }

        return metadata;
    }

    static std::string ExtractModuleFromVersion(const std::string& module_name, const std::string& full_version)
    {
        std::stringstream version_stream(full_version);
        std::string section;
        while (std::getline(version_stream, section, '|'))
        {
            if (section.find(module_name) != std::string::npos)
            {
                auto pos = section.find(':');
                auto sub = section.substr(pos + 1, std::string::npos);
                return sub;
            }
        }
        return "Unknown";
    }

    explicit operator std::string() const
    {
        std::ostringstream out;
        out << "file_path=" << _file_path << ", ";
        out << "port=" << _port;
        return out.str();
    }

    friend std::ostream& operator<<(std::ostream& out, const FWUpdaterPy& obj)
    {
        return out << static_cast<std::string>(obj);
    }
};

void init_fw_updater(pybind11::module& m)
{
    using namespace RealSenseID;
    py::register_exception<InvalidFirmwareException>(m, "InvalidFirmwareException", PyExc_RuntimeError);
    py::register_exception<SKUMismatchException>(m, "SKUMismatchException", PyExc_RuntimeError);
    py::register_exception<FWUpdatePolicyException>(m, "FWUpdatePolicyException", PyExc_RuntimeError);
    py::register_exception<IncompatibleHostException>(m, "IncompatibleHostException", PyExc_RuntimeError);
    py::register_exception<FWUpdateException>(m, "FWUpdateException", PyExc_RuntimeError);

    py::class_<FirmwareBinInfo, std::shared_ptr<FirmwareBinInfo>>(m, "FirmwareBinInfo")
        .def(py::init<>())
        .def_readonly("fw_version", &FirmwareBinInfo::fw_version)
        .def_readonly("recognition_version", &FirmwareBinInfo::recognition_version)
        .def_readonly("module_names", &FirmwareBinInfo::module_names)
        .def("__repr__", [](const FirmwareBinInfo& fp) {
            std::stringstream module_names;
            std::copy(fp.module_names.begin(), fp.module_names.end(), std::ostream_iterator<std::string>(module_names, ", "));
            std::ostringstream oss;
            oss << "<rsid_py.FirmwareBinInfo "
                << "fw_version=" << fp.fw_version << ", "
                << "recognition_version=" << fp.recognition_version << ", "
                << "module_names=[" << module_names.str() << "]>";
            return oss.str();
        });

    py::class_<DeviceMetadata, std::shared_ptr<DeviceMetadata>>(m, "DeviceFirmwareInfo")
        .def(py::init<>())
        .def_readonly("fw_version", &DeviceMetadata::fw_version)
        .def_readonly("recognition_version", &DeviceMetadata::recognition_version)
        .def_readonly("serial_number", &DeviceMetadata::serial_number)
        .def("__repr__", [](const DeviceMetadata& fp) {
            std::ostringstream oss;
            oss << "<rsid_py.DeviceFirmwareInfo "
                << "fw_version=" << fp.fw_version << ", "
                << "recognition_version=" << fp.recognition_version << ", "
                << "serial_number=" << fp.serial_number << ">";
            return oss.str();
        });


    py::class_<FWUpdaterPy, std::shared_ptr<FWUpdaterPy>>(m, "FWUpdater")
        .def(py::init([]() { return nullptr; })) // NOOP __init__
        .def(py::init<>([](const std::string& file_path, const std::string& port) {
                 auto updater = std::make_shared<FWUpdaterPy>(file_path, port);
                 return updater;
             }),
             py::arg("file_path").none(false), py::arg("port").none(false))
        .def("__enter__", [](FWUpdaterPy& self) { return &self; })
        .def("__exit__", &FWUpdaterPy::exit)
        .def("__repr__",
             [](const FWUpdaterPy& fp) {
                 std::ostringstream oss;
                 oss << "<rsid_py.FWUpdater " << fp << ">";
                 return oss.str();
             })

        // The following methods will not throw exceptions. The output will be in the form
        // (bool, message) = function()
        // if bool = true: message will be None
        // if bool = false: message will have meaningful output to display
        .def("is_sku_compatible", &FWUpdaterPy::IsSkuCompatible,
             R"docstring(
             Verify if firmware file is compatible with connected device.
             Returns
             ----------
             tuple[bool, str]
                 (is_compatible: bool, message: str) - if is_compatible == False, message will
                 indicate error messages or explanation.
             )docstring",
             py::call_guard<py::gil_scoped_release>())

        .def("is_host_compatible", &FWUpdaterPy::IsHostCompatible,
             R"docstring(
             Verify if firmware file is compatible with current host SDK version.
             Returns
             ----------
             tuple[bool, str]
                 (is_compatible: bool, message: str) - if is_compatible == False, message will
                 indicate error messages or explanation.
             )docstring",
             py::call_guard<py::gil_scoped_release>())

        .def("is_policy_compatible", &FWUpdaterPy::IsPolicyCompatible,
             R"docstring(
             Verify if firmware file is compatible with the update policy. In some situations, you
             may need to perform an intermediate update to an older firmware that the latest before
             you can apply the latest update.
             Returns
             ----------
             tuple[bool, str]
                 (is_compatible: bool, message: str) - if is_compatible == False, message will
                 indicate error messages or explanation.
             )docstring",
             py::call_guard<py::gil_scoped_release>())

        // The following methods will throw exceptions on errors. The exception messages will be exactly
        // equivalent to the messages you would see on calling the previous informative functions.
        // The exceptions are located at the top of this file and can be handled by catching (excepting) the
        // exceptions in the Python code.
        .def("get_firmware_bin_info", &FWUpdaterPy::GetFirmwareBinInfo,
             R"docstring(
             Retrieve firmware file info.
             Returns
             ----------
             FirmwareBinInfo
                 Class containing version information for firmware file contents.
             Raises
             ------
             InvalidFirmwareException
                 if the firmware file is invalid or corrupt.
             )docstring",
             py::call_guard<py::gil_scoped_release>())
        .def("get_device_firmware_info", &FWUpdaterPy::GetDeviceFirmwareInfo,
             R"docstring(
             Retrieve device firmware info.
             Returns
             ----------
             DeviceFirmwareInfo
                 Class containing version information for the running/current device firmware.
             Raises
             ------
             RuntimeError
                 if reading device info is not possible.
             )docstring",
             py::call_guard<py::gil_scoped_release>())

        .def("update", &FWUpdaterPy::Update, py::arg("force_version") = false, py::arg("force_full") = false,
             py::arg("progress_callback") = nullptr,
             R"docstring(
             Update the device to the firmware file provided.
             Parameters
             ----------
             force_version: bool
                 If the host and new versions are a mismatch, you will need to specify this flag
                 in order to forcefully perform the update.
                 defaults to False
             force_full: bool
                 Force update of all modules even if they already exist in the current device firmware.
             progress_callback: function(progress: float)
                 Callback function to receive updates of the update
                 progress. The progress is a float value between 0 and 1

             Examples
             --------
                 def progress_callback(progress: float):
                     logger.info(f"progress: {progress}")
                 updater.update(progress_callback=progress_callback)

             Returns
             ----------
             Status
                 Status for starting the update. Status.OK means that the update started and you should start
                 receiving callbacks on the progress_callback method.

             Raises
             ------
             InvalidFirmwareException
                 Firmware file is corrupt or invalid
             SKUMismatchException
                 Firmware file is incompatible with this device
             IncompatibleHostException
                 Firmware update needs to be forced as the host is incompatible
             FWUpdatePolicyException
                 Review exception message for policy requirements
             FWUpdateException
                 Generic firmware update exception
             RuntimeError
                 Generic firmware exception
             )docstring",
             py::call_guard<py::gil_scoped_release>());
}
