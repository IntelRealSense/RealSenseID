// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#include "FaceAuthenticatorF46x.h"
#include "Logger.h"

static const char* LOG_TAG = "FaceAuthenticatorF46x";

namespace RealSenseID
{
namespace Impl
{

FaceAuthenticatorF46x::FaceAuthenticatorF46x() : FaceAuthenticatorCommon(nullptr /* signature callback, not supported */)
{
}

} // namespace Impl
} // namespace RealSenseID
