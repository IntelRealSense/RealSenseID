// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#pragma once

#include "ModuleInfo.h"
#include <cstdint>
#include <string>

namespace RealSenseID
{
namespace FwUpdateF45x
{
// Parses a packaged binary firmware file and returns a list of modules with their metadata
ModuleVector ParseUfifToModules(const std::string& path, const uint32_t block_size);

// Parses a packaged binary firmware file and returns the encryption version used in it.
uint8_t ParseUfifToOtpEncryption(const std::string& path);

} // namespace FwUpdateF45x
} // namespace RealSenseID