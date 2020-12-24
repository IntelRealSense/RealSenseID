// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "PreviewImpl.h"

namespace RealSenseID
{
PreviewImpl::PreviewImpl(const PreviewConfig& config) : _preview {config}
{
}

PreviewImpl::~PreviewImpl()
{
    try
    {
        _preview.Stop();
    }
    catch (...)
    {
    }
}

bool PreviewImpl::StartPreview(PreviewImageReadyCallback& callback)
{
    _preview.Start(callback);
    return true;
}

bool PreviewImpl::PausePreview()
{
    _preview.Pause();
    return true;
}

bool PreviewImpl::ResumePreview()
{
    _preview.Resume();
    return true;
}

bool PreviewImpl::StopPreview()
{
    _preview.Stop();
    return true;
}
} // namespace RealSenseID
