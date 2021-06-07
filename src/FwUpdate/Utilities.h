// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#pragma once

#include "ModuleInfo.h"
#include <cstdint>
#include <string>

namespace RealSenseID
{
namespace FwUpdate
{
// Calculates crc on a data buffer
uint32_t CalculateCRC(uint32_t crc, const void* buffer, uint32_t buffer_size);

// Parses a packaged binary firmware file and returns a list of modules with their metadata
ModuleVector ParseUfifToModules(const std::string& path, const uint32_t block_size);

// Parses a packaged binary firmware file and returns the encryption version used in it.
uint8_t ParseUfifToOtpEncryption(const std::string& path);

// Load data of specified size and from specified offset into a buffer of an aligned size
std::vector<unsigned char> LoadFileToBuffer(const std::string& path, size_t aligned_size, size_t size, size_t offset);
} // namespace FwUpdate
} // namespace RealSenseID