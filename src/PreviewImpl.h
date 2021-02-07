// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/Preview.h"
#ifdef ANDROID
#include "AndroidPreview.h"
#else
#include "OpencvPreview.h"
#endif

namespace RealSenseID
{
class PreviewImpl
{
public:
    ~PreviewImpl();
    explicit PreviewImpl(const PreviewConfig& config);
    bool StartPreview(PreviewImageReadyCallback& callback);
    bool PausePreview();
    bool ResumePreview();
    bool StopPreview();

private:
	#ifdef ANDROID
	AndroidPreview _preview;
	#else
	OpencvPreview _preview;
	#endif
    
};
} // namespace RealSenseID
