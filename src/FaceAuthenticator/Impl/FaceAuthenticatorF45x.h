// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "FaceAuthenticatorCommon.h"

namespace RealSenseID
{
namespace Impl
{
class FaceAuthenticatorF45x : public FaceAuthenticatorCommon
{
public:
    explicit FaceAuthenticatorF45x(SignatureCallback* callback = nullptr);
};

} // namespace Impl
} // namespace RealSenseID
