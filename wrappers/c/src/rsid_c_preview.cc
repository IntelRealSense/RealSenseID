// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/Preview.h"
#include "rsid_c/rsid_preview.h"
#include <memory>

namespace
{
class PreviewClbk : public RealSenseID::PreviewImageReadyCallback
{
public:
    explicit PreviewClbk(rsid_preview_clbk c_clbk, void* ctx) : m_callback {c_clbk}, m_ctx {ctx}
    {
    }

    void OnPreviewImageReady(const RealSenseID::Image image) override
    {
        if (m_callback)
        {
            rsid_image c_img;
            c_img.buffer = image.buffer;
            c_img.size = image.size;
            c_img.width = image.width;
            c_img.height = image.height;
            c_img.stride = image.stride;
            c_img.number = image.number;
            m_callback(c_img, m_ctx);
        }
    }

private:
    rsid_preview_clbk m_callback;
    void* m_ctx;
};
} // namespace

static std::unique_ptr<PreviewClbk> s_preview_clbk;

rsid_preview* rsid_create_preview(const rsid_preview_config* preview_config)
{
    RealSenseID::PreviewConfig config;
    config.cameraNumber = preview_config->camera_number;
    config.debugMode = preview_config->debug_mode;
    auto* preview_impl = new RealSenseID::Preview(config);

    if (preview_impl == nullptr)
    {
        return nullptr;
    }

    auto* rv = new rsid_preview();
    rv->_impl = preview_impl;
    return rv;
}

void rsid_destroy_preview(rsid_preview* preview_handle)
{
    if (!preview_handle)
        return;

    try
    {
        auto* preview_impl = static_cast<RealSenseID::Preview*>(preview_handle->_impl);
        delete preview_impl;
        s_preview_clbk.reset();
    }
    catch (...)
    {
    }
    delete preview_handle;
}

int rsid_start_preview(rsid_preview* preview_handle, rsid_preview_clbk clbk, void* ctx)
{
    if (!preview_handle)
        return 0;

    if (!preview_handle->_impl)
        return 0;

    try
    {
        auto* preview_impl = static_cast<RealSenseID::Preview*>(preview_handle->_impl);
        s_preview_clbk = std::make_unique<PreviewClbk>(clbk, ctx);
        bool ok = preview_impl->StartPreview(*s_preview_clbk);
        return static_cast<int>(ok);
    }
    catch (...)
    {
        return 0;
    }
}


int rsid_pause_preview(rsid_preview* preview_handle)
{
    if (!preview_handle)
        return 0;

    if (!preview_handle->_impl)
        return 0;

    try
    {
        auto* preview_impl = static_cast<RealSenseID::Preview*>(preview_handle->_impl);
        bool ok = preview_impl->PausePreview();
        return static_cast<int>(ok);
    }
    catch (...)
    {
        return 0;
    }
}

int rsid_resume_preview(rsid_preview* preview_handle)
{
    if (!preview_handle)
        return 0;

    if (!preview_handle->_impl)
        return 0;

    try
    {
        auto* preview_impl = static_cast<RealSenseID::Preview*>(preview_handle->_impl);
        bool ok = preview_impl->ResumePreview();
        return static_cast<int>(ok);
    }
    catch (...)
    {
        return 0;
    }
}


int rsid_stop_preview(rsid_preview* preview_handle)
{
    if (!preview_handle)
        return 0;

    if (!preview_handle->_impl)
        return 0;

    try
    {
        auto* preview_impl = static_cast<RealSenseID::Preview*>(preview_handle->_impl);
        bool ok = preview_impl->StopPreview();
        return static_cast<int>(ok);
    }
    catch (...)
    {
        return 0;
    }
}
