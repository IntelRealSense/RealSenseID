// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#pragma once

#include "FwUpdaterComm.h"
#include "ModuleInfo.h"
#include <string>
#include <functional>
#include <vector>

namespace RealSenseID
{
namespace FwUpdate
{
class FwUpdateEngine
{
public:
    using ProgressCallback = std::function<void(float)>;
    using ProgressTick = std::function<void()>;
    using Buffer = std::vector<unsigned char>;

    struct Settings
    {
        static const long DefaultBaudRate = 115200;

        std::string fw_filename;
        const char* port = nullptr;
        long baud_rate = DefaultBaudRate;
        bool force_full = false; // if true update all modules and blocks regardless of crc checks
    };

    FwUpdateEngine() = default;
    ~FwUpdateEngine() = default;
    ModuleVector ModulesFromFile(const std::string& filename);
    void BurnModules(const Settings& settings, const ModuleVector& modules, ProgressCallback on_progress);

private:
    static constexpr const uint32_t BlockSize = 512 * 1024;
    std::unique_ptr<FwUpdaterComm> _comm;

    struct ModuleVersionInfo;
    std::vector<ModuleVersionInfo> ModulesFromDevice();
    ModuleVersionInfo ModuleFromDevice(const std::string& module_name);
    void CleanObsoleteModules(const std::vector<ModuleInfo>& file_modules, const std::vector<ModuleVersionInfo>& device_modules);
    void InitNewModules(const std::vector<ModuleInfo>& file_modules, const std::vector<ModuleVersionInfo>& device_modules);
    void BurnSelectModules(const ModuleVector& modules, ProgressTick tick, bool force_full);
    void BurnModule(ProgressTick tick, const ModuleInfo& module, const Buffer& buffer, bool is_first, bool is_last, bool force_full);
    std::vector<bool> GetBlockUpdateList(const ModuleInfo& module, bool force_full);
    bool ParseDlResponse(const std::string& name, size_t blkNo, size_t sz);
    bool ParseDlBlockResult();
};
} // namespace FwUpdate
} // namespace RealSenseID
