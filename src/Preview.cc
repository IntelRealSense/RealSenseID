// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/Preview.h"
#include "PreviewImpl.h"

namespace RealSenseID
{
Preview::Preview(const PreviewConfig& config) : _impl {new PreviewImpl {config}}
{
}

Preview::~Preview()
{
    try
    {
        delete _impl;
    }
    catch (...)
    {
    }
}

bool Preview::StartPreview(PreviewImageReadyCallback& callback)
{
    return _impl->StartPreview(callback);
}

bool Preview::PausePreview()
{
    return _impl->PausePreview();
}

bool Preview::ResumePreview()
{
    return _impl->ResumePreview();
}

bool Preview::StopPreview()
{
    return _impl->StopPreview();
}

bool Preview::RawToRgb(const Image& in_image, Image& out_image)
{
    return _impl->RawToRgb(in_image, out_image);
}
} // namespace RealSenseID
