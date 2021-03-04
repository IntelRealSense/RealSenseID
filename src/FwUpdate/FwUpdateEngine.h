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

    struct ModuleVersionInfo;

    // do complete fw update session
    void Session(const ModuleVector& modules, ProgressTick progress_tick, bool force_full);

    // update single module
    void BurnModule(ProgressTick tick, const ModuleInfo& module, const Buffer& buffer, bool is_first, bool is_last,
                    bool force_full);

    std::vector<bool> GetBlockUpdateList(const ModuleInfo& module, bool force_full);

    bool ConsumeDlVerResponse(const std::string& module_name, ModuleVersionInfo& module_info);
    bool ParseDlResponse(const std::string& name, size_t blkNo, size_t sz);
    bool ParseDlVer(const char* input, const std::string& module_name, ModuleVersionInfo& result);
    bool ParseDlBlockResult();

    std::unique_ptr<FwUpdaterComm> _comm;
};
} // namespace FwUpdate
} // namespace RealSenseID
