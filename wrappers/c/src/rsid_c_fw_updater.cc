// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/FwUpdater.h"
#include "rsid_c/rsid_fw_updater.h"
#include <algorithm>
#include <string>
#include <thread>
#include <chrono>

namespace
{
    static constexpr int WAIT_FOR_DEVICE_REBOOT_MS = 7000;
    static const std::string OPFW = "OPFW";
    static const std::string RECOG = "RECOG";
    
class FwUpdaterEventHandler : public RealSenseID::FwUpdater::EventHandler
{
public:
    explicit FwUpdaterEventHandler(const rsid_fw_update_event_handler* c_clbk, float minValue, float maxValue) :
        m_callback {c_clbk}, m_minValue(minValue), m_maxValue(maxValue)
    {
    }

    void OnProgress(float progress) override
    {
        if (m_callback->progress_callback)
            m_callback->progress_callback(m_minValue + progress * (m_maxValue - m_minValue));
    }

private:
    const rsid_fw_update_event_handler* m_callback;
    float m_minValue, m_maxValue;
};
} // namespace

rsid_fw_updater* rsid_create_fw_updater()
{
    auto* fw_updater = new RealSenseID::FwUpdater();
    if (fw_updater == nullptr)
        return nullptr;

    auto* rv = new rsid_fw_updater();
    rv->_impl = fw_updater;
    return rv;
}

void rsid_destroy_fw_updater(rsid_fw_updater* handle)
{
    if (!handle)
        return;

    try
    {
        auto* fw_updater_impl = static_cast<RealSenseID::FwUpdater*>(handle->_impl);
        delete fw_updater_impl;
    }
    catch (...)
    {
    }

    delete handle;
}

int rsid_extract_firmware_version(rsid_fw_updater* handle, const char* bin_path, char* new_fw_version,
                                  size_t new_fw_version_length, char* new_recognition_version,
                                  size_t new_recognition_version_size)
{
    auto* fw_updater_impl = static_cast<RealSenseID::FwUpdater*>(handle->_impl);

    std::string out_fw_version;
    std::string out_recognition_version;
    std::vector<std::string> moduleNames;
    bool success = fw_updater_impl->ExtractFwInformation(bin_path, out_fw_version, out_recognition_version, moduleNames);

    if (!success)
        return rsid_status::RSID_Error;

    if (out_fw_version.length() >= new_fw_version_length ||
        out_recognition_version.length() >= new_recognition_version_size)
        return rsid_status::RSID_Error;

    ::strncpy(new_fw_version, out_fw_version.c_str(), new_fw_version_length);
    ::strncpy(new_recognition_version, out_recognition_version.c_str(), new_recognition_version_size);
    return rsid_status::RSID_Ok;
}

rsid_status rsid_update_firmware(rsid_fw_updater* handle, const rsid_fw_update_event_handler* event_handler,
                                 rsid_fw_update_settings settings, const char* bin_path, int update_recognition)
{
    auto* fw_updater_impl = static_cast<RealSenseID::FwUpdater*>(handle->_impl);

    RealSenseID::FwUpdater::Settings fw_updater_settings;
    fw_updater_settings.port = settings.port;
    fw_updater_settings.force_full = settings.force_full;
    
    std::string out_fw_version;
    std::string out_recognition_version;
    std::vector<std::string> moduleNames;
    bool success = fw_updater_impl->ExtractFwInformation(bin_path, out_fw_version, out_recognition_version, moduleNames);
    if (!success)
        return rsid_status::RSID_Error;
    if (!update_recognition)
    {
        moduleNames.erase(std::remove_if(moduleNames.begin(), moduleNames.end(), [](const std::string& moduleName) { return moduleName.compare(RECOG) == 0; }), moduleNames.end());
    }
    auto numberOfModules = moduleNames.size();
    FwUpdaterEventHandler eh1(event_handler, 0.f, 1.f / numberOfModules);
    // Temprorarily disable two step installing. Flash all modules sequentially.
    /**
    std::vector<std::string> modulesVector;
    modulesVector.push_back(OPFW);
    RealSenseID::Status s = fw_updater_impl->UpdateModules(&eh1, fw_updater_settings, bin_path, modulesVector);
    if (s != RealSenseID::Status::Ok)
        return static_cast<rsid_status>(s);
    std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_FOR_DEVICE_REBOOT_MS));
    moduleNames.erase(std::remove_if(moduleNames.begin(), moduleNames.end(), [](const std::string& moduleName) { return moduleName.compare(OPFW) == 0; }), moduleNames.end());
    FwUpdaterEventHandler eh2(event_handler, 1.f / numberOfModules, 1.f);
    return static_cast<rsid_status>(fw_updater_impl->UpdateModules(&eh2, fw_updater_settings, bin_path, moduleNames));
    **/
    FwUpdaterEventHandler eh2(event_handler, 0.f, 1.f);
    return static_cast<rsid_status>(fw_updater_impl->UpdateModules(&eh2, fw_updater_settings, bin_path, moduleNames));
}

int rsid_is_encryption_compatible_with_device(rsid_fw_updater* handle, const char* bin_path, const char* serial_number)
{
    auto* fw_updater_impl = static_cast<RealSenseID::FwUpdater*>(handle->_impl);
    return fw_updater_impl->IsEncryptionSupported(bin_path, serial_number);
}
