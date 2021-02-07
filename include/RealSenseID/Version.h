// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseIDExports.h"

#define RSID_VER_MAJOR 0
#define RSID_VER_MINOR 11
#define RSID_VER_PATCH 0

#define RSID_VERSION (RSID_VER_MAJOR * 10000 + RSID_VER_MINOR * 100 + RSID_VER_PATCH)

namespace RealSenseID
{
/**
 * Library version as semver string
 */
RSID_API const char* Version();
} // namespace RealSenseID