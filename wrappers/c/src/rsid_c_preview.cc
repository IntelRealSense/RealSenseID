// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/Preview.h"
#include "rsid_c/rsid_preview.h"
#include <memory>

namespace
{

RealSenseID::Image c_img_to_api_image(const rsid_image* in_img)
{
    RealSenseID::Image out_img;
    out_img.buffer = in_img->buffer;
    out_img.size = in_img->size;
    out_img.width = in_img->width;
    out_img.height = in_img->height;
    out_img.stride = in_img->stride;
    out_img.number = in_img->number;
    out_img.metadata.led = in_img->metadata.led;
    out_img.metadata.projector = in_img->metadata.projector;
    out_img.metadata.sensor_id = in_img->metadata.sensor_id;
    out_img.metadata.status = in_img->metadata.status;
    out_img.metadata.timestamp = in_img->metadata.timestamp;
    return out_img;
}

rsid_image api_image_to_c_img(const RealSenseID::Image* in_img)
{
    rsid_image out_img;
    out_img.buffer = in_img->buffer;
    out_img.size = in_img->size;
    out_img.width = in_img->width;
    out_img.height = in_img->height;
    out_img.stride = in_img->stride;
    out_img.number = in_img->number;
    out_img.metadata.led = in_img->metadata.led;
    out_img.metadata.projector = in_img->metadata.projector;
    out_img.metadata.sensor_id = in_img->metadata.sensor_id;
    out_img.metadata.status = in_img->metadata.status;
    out_img.metadata.timestamp = in_img->metadata.timestamp;
    return out_img;
}

class PreviewClbk : public RealSenseID::PreviewImageReadyCallback
{
public:
    explicit PreviewClbk(rsid_preview_clbk c_clbk_preview, rsid_preview_clbk c_clbk_snapshot, void* ctx) :
        m_callback_preview {c_clbk_preview}, m_callback_snapshot(c_clbk_snapshot), m_ctx {ctx}
    {
    }

    explicit PreviewClbk(rsid_preview_clbk c_clbk_preview, void* ctx) :
        m_callback_preview {c_clbk_preview}, m_ctx {ctx}
    {
    }

    void OnPreviewImageReady(const RealSenseID::Image image) override
    {
        if (m_callback_preview)
        {
            m_callback_preview(api_image_to_c_img(&image), m_ctx);
        }
    }

    void OnSnapshotImageReady(const RealSenseID::Image image) override
    {
        if (m_callback_snapshot)
        {
            m_callback_snapshot(api_image_to_c_img(&image), m_ctx);
        }
    }

private:
    rsid_preview_clbk m_callback_preview = NULL;
    rsid_preview_clbk m_callback_snapshot = NULL;
    void* m_ctx;
};
} // namespace

static std::unique_ptr<PreviewClbk> s_preview_clbk;

rsid_preview* rsid_create_preview(const rsid_preview_config* preview_config)
{
    RealSenseID::PreviewConfig config;
    config.cameraNumber = preview_config->camera_number;
    config.previewMode = static_cast<RealSenseID::PreviewMode>(preview_config->preview_mode);
    config.portraitMode = static_cast<bool>(preview_config->portraitMode);
    config.rotateRaw = static_cast<bool>(preview_config->rotateRaw);
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

int rsid_start_preview(rsid_preview* preview_handle, rsid_preview_clbk clbk_preview, void* ctx)
{
    if (!preview_handle)
        return 0;

    if (!preview_handle->_impl)
        return 0;

    try
    {
        auto* preview_impl = static_cast<RealSenseID::Preview*>(preview_handle->_impl);
        s_preview_clbk = std::make_unique<PreviewClbk>(clbk_preview, ctx);
        bool ok = preview_impl->StartPreview(*s_preview_clbk);
        return static_cast<int>(ok);
    }
    catch (...)
    {
        return 0;
    }
}

int rsid_start_preview_and_snapshots(rsid_preview* preview_handle, rsid_preview_clbk clbk_preview, rsid_preview_clbk clbk_snapshots, void* ctx)
{
    if (!preview_handle)
        return 0;

    if (!preview_handle->_impl)
        return 0;

    try
    {
        auto* preview_impl = static_cast<RealSenseID::Preview*>(preview_handle->_impl);
        s_preview_clbk = std::make_unique<PreviewClbk>(clbk_preview, clbk_snapshots, ctx);
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