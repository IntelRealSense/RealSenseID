// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/FwUpdater.h"
#include "RealSenseID/DeviceController.h"
#include "RealSenseID/DiscoverDevices.h"
#include "RealSenseID/SerialConfig.h"
#include "rsid_c/rsid_fw_updater.h"
#include <algorithm>
#include <string>
#include <thread>
#include <chrono>
#include <cassert>

namespace
{
static constexpr int MIN_WAIT_FOR_DEVICE_REBOOT_SEC = 6;
static constexpr int MAX_WAIT_FOR_DEVICE_REBOOT_SEC = 30;
static const std::string OPFW = "OPFW";

struct device_info_wrapper
{
    std::unique_ptr<RealSenseID::DeviceInfo> config;
};

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

rsid_fw_updater* rsid_create_fw_updater(rsid_device_type device_type)
{
    auto* fw_updater = new RealSenseID::FwUpdater(static_cast<RealSenseID::DeviceType>(device_type));
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

rsid_status rsid_extract_firmware_version(rsid_fw_updater* handle, const char* bin_path, char* new_fw_version, size_t new_fw_version_length,
                                          char* new_recognition_version, size_t new_recognition_version_size)
{
    const auto* fw_updater_impl = static_cast<RealSenseID::FwUpdater*>(handle->_impl);

    std::string out_fw_version;
    std::string out_recognition_version;
    std::vector<std::string> moduleNames;
    bool success = fw_updater_impl->ExtractFwInformation(bin_path, out_fw_version, out_recognition_version, moduleNames);

    if (!success)
        return rsid_status::RSID_Error;

    if (out_fw_version.length() >= new_fw_version_length || out_recognition_version.length() >= new_recognition_version_size)
        return rsid_status::RSID_Error;

    ::strncpy(new_fw_version, out_fw_version.c_str(), new_fw_version_length);
    ::strncpy(new_recognition_version, out_recognition_version.c_str(), new_recognition_version_size);
    return rsid_status::RSID_Ok;
}


rsid_status rsid_update_firmware(rsid_fw_updater* handle, const rsid_fw_update_event_handler* event_handler,
                                 rsid_fw_update_settings settings, const char* bin_path)
{
    const auto* fw_updater_impl = static_cast<RealSenseID::FwUpdater*>(handle->_impl);

    RealSenseID::FwUpdater::Settings fw_updater_settings;
    fw_updater_settings.serial_config = RealSenseID::SerialConfig({settings.port});
    fw_updater_settings.force_full = settings.force_full;

    FwUpdaterEventHandler eh(event_handler, 0.f, 1.f);
    RealSenseID::Status s = fw_updater_impl->UpdateModules(&eh, fw_updater_settings, bin_path);
    return static_cast<rsid_status>(s);
}

// Return if sku compatibbe and set the values pointed by expected_sku_ver_ptr and device_sku_ver_ptr pointers
int rsid_is_sku_compatible(rsid_fw_updater* handle, rsid_fw_update_settings settings, const char* bin_path, int* expected_sku_ver_ptr,
                           int* device_sku_ver_ptr)
{
    assert(expected_sku_ver_ptr != nullptr);
    assert(device_sku_ver_ptr != nullptr);
    if (expected_sku_ver_ptr == nullptr || device_sku_ver_ptr == nullptr)
    {
        return 0;
    }
    const auto* fw_updater_impl = static_cast<RealSenseID::FwUpdater*>(handle->_impl);
    RealSenseID::FwUpdater::Settings fw_updater_settings;
    fw_updater_settings.serial_config = RealSenseID::SerialConfig({settings.port});
    int expected_sku_ver = 0;
    int device_sku_ver = 0;
    auto rv = fw_updater_impl->IsSkuCompatible(fw_updater_settings, bin_path, expected_sku_ver, device_sku_ver);
    *expected_sku_ver_ptr = expected_sku_ver;
    *device_sku_ver_ptr = device_sku_ver;
    return rv;
}
