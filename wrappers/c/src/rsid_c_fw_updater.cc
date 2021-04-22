// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/FwUpdater.h"
#include "rsid_c/rsid_fw_updater.h"
#include <string>

namespace
{
class FwUpdaterEventHandler : public RealSenseID::FwUpdater::EventHandler
{
public:
    explicit FwUpdaterEventHandler(const rsid_fw_update_event_handler* c_clbk) : m_callback {c_clbk}
    {
    }

    void OnProgress(float progress) override
    {
        if (m_callback->progress_callback)
            m_callback->progress_callback(progress);
    }

private:
    const rsid_fw_update_event_handler* m_callback;
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
    bool success = fw_updater_impl->ExtractFwVersion(bin_path, out_fw_version, out_recognition_version);

    if (!success)
        return false;

    if (out_fw_version.length() >= new_fw_version_length ||
        out_recognition_version.length() >= new_recognition_version_size)
        return rsid_status::RSID_Error;

    ::strncpy(new_fw_version, out_fw_version.c_str(), new_fw_version_length);
    ::strncpy(new_recognition_version, out_recognition_version.c_str(), new_recognition_version_size);
    return rsid_status::RSID_Ok;
}

rsid_status rsid_update_firmware(rsid_fw_updater* handle, const rsid_fw_update_event_handler* event_handler,
                                 rsid_fw_update_settings settings, const char* bin_path, int exclude_recognition)
{
    auto* fw_updater_impl = static_cast<RealSenseID::FwUpdater*>(handle->_impl);

    RealSenseID::FwUpdater::Settings fw_updater_settings;
    fw_updater_settings.port = settings.port;
    fw_updater_settings.force_full = settings.force_full;
    FwUpdaterEventHandler eh(event_handler);

    return static_cast<rsid_status>(fw_updater_impl->Update(&eh, fw_updater_settings, bin_path, exclude_recognition));
}
