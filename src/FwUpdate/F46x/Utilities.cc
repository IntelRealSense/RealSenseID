// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#include "Utilities.h"
#include "../Common/Common.h"
#include "Logger.h"
#include <fstream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <regex>
#include <sstream>

namespace RealSenseID
{
namespace FwUpdateF46x
{
static const char* LOG_TAG = "FwUpdateF46x";

static constexpr uint32_t UFIF_ALIGN = 16;
static constexpr uint32_t UFIF_SIG = 0x46484655;
static constexpr uint32_t UFIF_VER = 0x0100;
static constexpr uint32_t UFIF_NAME_MAX = 64;
static constexpr uint32_t DIGEST_HEADER_VERSION = 0x00000004;
static constexpr uint32_t DIGEST_HEADER_VERSION_SIZE = 12;

// Header of each module
struct DigestHeader
{
    uint32_t ModuleHeader;
    uint32_t HeaderLen;
    uint32_t HeaderVersion;
    uint32_t ModuleID;
    uint32_t ModuleVendor;
    uint32_t Date;
    uint32_t Size;
    uint32_t PublicKeyLen;
    uint32_t SigRLen;
    uint32_t SigSLen;
    uint8_t Reserved[88];
    uint8_t Qx[0x10 * 2];
    uint8_t Qy[0x10 * 2];
    uint8_t signature[64];
    uint32_t ver;
    uint8_t id[8];
    uint8_t binVer[DIGEST_HEADER_VERSION_SIZE];
    uint32_t flags;
    uint32_t binSize;
    uint8_t iv[16];
    uint32_t orgSize;
    uint8_t padding[204];
};

struct UfifEntry
{
    char name[UFIF_NAME_MAX];
    uint32_t size;
    uint32_t crc32;
    uint8_t rsv[8];
};

struct UfifFile
{
    uint32_t sig;
    uint16_t ver;
    uint16_t entryN;
    uint8_t otpEncryptVersion;
    uint8_t rsv[23];
};

static bool UfifCheckHeader(const UfifFile& header)
{
    if (header.sig != UFIF_SIG || (header.ver >> 8) != (UFIF_VER >> 8))
    {
        LOG_TRACE(LOG_TAG, "ufif header err, sig:%x != %x, ver:%x != %x", header.sig, UFIF_SIG, header.ver >> 8, UFIF_VER >> 8);
        return false;
    }
    return true;
}

static std::vector<UfifEntry> UfifReadHeader(std::ifstream& file, UfifFile& header)
{
    if (!file.read((char*)&header, sizeof(UfifFile)))
    {
        throw std::runtime_error("Error while reading ufifFile_t");
    }


    if (!UfifCheckHeader(header))
    {
        throw std::runtime_error("Error while validating ufif header");
    }

    // read headers entries from file
    std::vector<UfifEntry> rv(header.entryN);
    if (!file.read((char*)rv.data(), rv.size() * sizeof(UfifEntry)))
    {
        throw std::runtime_error("Error while reading ufifEntries");
    }
    return rv;
}

uint8_t ParseUfifToOtpEncryption(const std::string& path)
{
    std::ifstream ifile(path, std::ios::binary);
    if (!ifile)
    {
        throw std::runtime_error("Error while trying to read project header");
    }
    ifile.unsetf(std::ios::skipws);

    UfifFile header;
    UfifReadHeader(ifile, header);
    return header.otpEncryptVersion;
}
ModuleVector ParseUfifToModules(const std::string& path, const uint32_t block_size)
{
    std::ifstream ifile(path, std::ios::binary);
    if (!ifile)
    {
        throw std::runtime_error("Error while trying to read project header");
    }
    ifile.unsetf(std::ios::skipws);

    UfifFile header;
    auto entries = UfifReadHeader(ifile, header);
    ModuleVector result;
    LOG_DEBUG(LOG_TAG, "Header Entry size = %d", entries.size());

    for (const auto& entry : entries)
    {
        auto ofs = ifile.tellg();
        if (ofs == -1)
        {
            throw std::runtime_error("tellg failed");
        }
        if (ofs % UFIF_ALIGN)
        {
            ofs += UFIF_ALIGN - ofs % UFIF_ALIGN;
            if (!ifile.seekg(ofs))
            {
                throw std::runtime_error("seekg failed");
            }
        }
        // 4k aligned module size
        auto aligned_buffer_size = (entry.size + 4095) & 0xfffff000;
        auto n_blocks = aligned_buffer_size / block_size;
        if (aligned_buffer_size % block_size)
        {
            n_blocks++;
        }

        std::string module_name;
        std::string module_file_name;
        std::string module_version;
        // regex groups to match: module_name, version, extension
        static const std::regex rgx {R"((.+)(SBC|NNLED|NNLEDR|DNET|RECOG|YOLO|AS2DLR|ASDISP|SPOOFS|ACCNET|ASVIS)\.([\d\.]+)\.(.+))",
                                     std::regex_constants::icase};
        std::smatch match;
        std::string line = std::string(entry.name, sizeof(entry.name) - 1);
        auto match_ok = std::regex_search(line, match, rgx);
        if (match_ok == false)
        {
            // Check for BOOT.INI file
            static const std::regex boot_rgx {R"((.+)(BOOT)\.(.+))"};
            auto match_ok = std::regex_search(line, match, boot_rgx);
            if (match_ok == false)
            {
                throw std::runtime_error("Invalid module name in " + line);
            }

            module_name = match[2].str();
            // module_file_name = module_name.<extension> : Example: BOOT.INI
            module_file_name = module_name + '.' + match[3].str();
        }
        else
        {
            module_name = match[2].str();
            module_version = match[3].str();
            if (module_name == "SBC")
            {
                module_name = "OPFW";
                // For OPFW maodule the file name is of format SBC.<W>.<X>.<Y>.<Z>.<ext>
                module_file_name = "SBC." + module_version + '.' + match[4].str();
            }
            else
            {
                // module_file_name = module_name.module_version.<extension> : Example: RECOG.2.5.24.0.sbin
                module_file_name = module_name + '.' + module_version + '.' + match[4].str();
            }
            assert(!module_version.empty());
        }
        assert(!module_name.empty());
        assert(!module_file_name.empty());
        // Convert to upper case for future comparisons.
        std::transform(module_file_name.begin(), module_file_name.end(), module_file_name.begin(), ::toupper);

        LOG_DEBUG(LOG_TAG, "[%s] %0.2f MB, %u blocks", module_file_name.c_str(), entry.size / 1048576.0, n_blocks);

        // buffer to store module data,
        // init with zeroes so alignment bytes(aligned_buffer_size-entry.size) are all zeroes
        std::vector<unsigned char> module_buffer(aligned_buffer_size, 0);
        if (!ifile.read((char*)&module_buffer[0], entry.size))
        {
            throw std::runtime_error("Failed reading module from file");
        }

        // crc sz must be 4-aligned
        uint32_t crc_aligned_data_size = (entry.size + 3) & ~3;
        auto whole_module_crc = FwUpdateCommon::CalculateCRC(0, module_buffer.data(), crc_aligned_data_size);
        if (whole_module_crc != entry.crc32)
        {
            throw std::runtime_error("Invalid crc field in module " + module_name);
        }

        ModuleInfo module_info;
        module_info.crc = whole_module_crc;
        module_info.name = module_name;
        module_info.version = module_version;
        module_info.filename = path;
        module_info.file_name = module_file_name;
        module_info.file_offset = ofs;
        module_info.size = entry.size;
        module_info.aligned_size = aligned_buffer_size;
        uint32_t block_crc_size = 0;
        size_t block_size_min = 0;
        for (unsigned i = 0; i < n_blocks; i++)
        {
            BlockInfo block;
            block.offset = i * static_cast<size_t>(block_size);
            block_size_min =
                (block.offset + static_cast<size_t>(block_size) > module_info.size) ? (module_info.size - block.offset) : block_size;
            block_crc_size = std::min(crc_aligned_data_size, block_size);
            block.size = std::min(static_cast<size_t>(block_crc_size), block_size_min);
            block.crc = FwUpdateCommon::CalculateCRC(i, &module_buffer[block.offset], block_crc_size);
            crc_aligned_data_size -= block_crc_size;
            module_info.blocks.push_back(block);
        }

        LOG_DEBUG(LOG_TAG, "Updated Entry Size = %d", module_info.size);

        result.push_back(module_info);
    }
    return result;
}

} // namespace FwUpdateF46x
} // namespace RealSenseID
