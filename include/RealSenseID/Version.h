// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseIDExports.h"

#include <string>

#define RSID_VER_MAJOR 0
#define RSID_VER_MINOR 42
#define RSID_VER_PATCH 0

#define RSID_VERSION (RSID_VER_MAJOR * 10000 + RSID_VER_MINOR * 100 + RSID_VER_PATCH)

#define RSID_FW_VER_MAJOR 7
#define RSID_FW_VER_MINOR 6

namespace RealSenseID
{
/**
 * Library version as semver string
 */
RSID_API const char* Version();

/**
 * Compatible firmware version as semver string (major and minor only)
 */
RSID_API const char* CompatibleFirmwareVersion();

/**
 * Checks if the firmware version is compatible with the host.
 *
 * @param[in] fw_version String containing firmware version.
 * @return True if compatible and false otherwise.
 */
RSID_API bool IsFwCompatibleWithHost(const std::string& fw_version);

} // namespace RealSenseID
