// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#include "FwUpdateEngine.h"
#include "Utilities.h"
#include "Logger.h"
#include "Cmds.h"
#include <thread>
#include <chrono>
#include <regex>
#include <sstream>
#include <cassert>
#include <set>
#include <algorithm>
#include <stdexcept>
#include <memory>
#include <cstring>

namespace RealSenseID
{
namespace FwUpdate
{
static const char* LOG_TAG = "FwUpdater";

static const std::set<std::string> AllowedModules {"OPFW", "NNLED", "NNLAS", "DNET", "RECOG", "YOLO", "AS2DLR"};
static const char* OPFW = "OPFW";

struct FwUpdateEngine::ModuleVersionInfo
{
    enum class State
    {
        Empty,
        Active,
        ActiveUpdating,
        Pending
    };

    std::string name;
    std::string version;
    State state;

    // return State enum from given string or throw if invalid
    static State StateFromString(const std::string& str)
    {
        if (str == "empty")
            return State::Empty;
        if (str == "active")
            return State::Active;
        if (str == "pending")
            return State::Pending;
        if (str == "active-updating")
            return State::ActiveUpdating;
        throw std::runtime_error("Invalid info state: \"" + str + "\"");
    }
};

// search for module_name for the given input(dlver response)
// return true if found valid line for this module, and fill the result struct
bool FwUpdateEngine::ParseDlVer(const char* input, const std::string& module_name, ModuleVersionInfo& result)
{
    // regex to find line of the form: OPFW : [OPFW] [0.0.0.1] (active)
    // regex groups to match: module_name, module_name, version, state
    static const std::regex rgx {
        R"((\w+) : \[(OPFW|NNLED|NNLAS|DNET|RECOG|YOLO|AS2DLR|SCRAP)\] \[([\d\.]+)\] \(([\w-]+)\))"};
    std::smatch match;

    // do regex on each line in the input and construct ModuleVersionInfo from it
    std::stringstream ss(input);
    std::string line;
    while (std::getline(ss, line, '\n'))
    {
        auto match_ok = std::regex_search(line, match, rgx);
        // find the line with the required module_name
        if (!match_ok || match[2] != module_name)
        {
            continue;
        }
        assert(match[1] == match[2]); // name : [name] should be same name
        result.name = match[2];
        result.version = match[3];
        result.state = ModuleVersionInfo::StateFromString(match[4]);

        LOG_DEBUG(LOG_TAG, "ParseDlVer(%s) result: name=%s, version=%s, state=%d", module_name.c_str(),
                  result.name.c_str(), result.version.c_str(), (int)result.state);
        return true;
    }
    return false;
}

bool FwUpdateEngine::ConsumeDlVerResponse(const std::string& module_name, ModuleVersionInfo& module_info)
{
    char* logBuf = _comm->GetScanPtr();

     LOG_DEBUG(LOG_TAG, "**************** ParseDlVer ********************");
     LOG_DEBUG(LOG_TAG, "%s", logBuf);
     LOG_DEBUG(LOG_TAG, "**************************************************");

    auto is_ok = ParseDlVer(logBuf, module_name, module_info);
    if (!is_ok)
    {
        LOG_ERROR(LOG_TAG, "Error, no info for module %s", module_name.c_str());
    }
    _comm->ConsumeScanned();
    return is_ok;
}

/*
parse dlinfo response

if amount of blocks in device/host are the same - if not, update all blocks

else, decide which blocks need to be updated according to -
compare pre-calcualted block crcs in ModuleInfo to the ones we receive from device (HDR CRC)
HDR CRC == REAL CRC == OUR CRC
status == "OK"
return vector of bools to flag if need update(true) or not(false)

example dlinfo response to parse:

FW is empty

 or

dlinfo ack
OPFW is active-updating
OPFW info
total 2921060, blkSz 524288
blk  state HDR CRC  Real CRC
#0   OK    b9fa1f11 b9fa1f11
#1   OK    fae50cb5 fae50cb5
#2   OK    c063d014 c063d014
#3   OK    1087f825 1087f825
#4   OK    962a6840 962a6840
#5   OK    06e7b8f7 06e7b8f7
OPFW end
SCRAP info
total 2921072, blkSz 524288
blk  state HDR CRC  Real CRC
#0   OK    aae66184 aae66184
#1   OK    509c4750 509c4750
#2   OK    2d7bd8bc 2d7bd8bc
#3   OK    dd3e1b79 dd3e1b79
#4   OK    1b66b124 1b66b124
#5   OK    3a197894 3a197894
SCRAP end
dlinfo end
*/
std::vector<bool> FwUpdateEngine::GetBlockUpdateList(const ModuleInfo& module, bool force_full)
{
    // all blocks should be updated until proven otherwise ("Ok" status and CRCs match: HDR==Real==Host)
    std::vector<bool> rv(module.blocks.size(), true);
    char* logBuf = _comm->GetScanPtr();

     LOG_DEBUG(LOG_TAG, "**************** dlinfo response ********************");
     LOG_DEBUG(LOG_TAG, "%s", logBuf);
     LOG_DEBUG(LOG_TAG, "*****************************************************");

    // if force_full or if "empty" encountered, all blocks need to be updated
    if (force_full || strstr(logBuf, "empty") != nullptr)
    {
        LOG_DEBUG(LOG_TAG, "Force update of all blocks");
        _comm->ConsumeScanned();
        return rv;
    }
    const char* cur_input = logBuf;
    // jump to SCRAP info section if exists
    const char* p = strstr(cur_input, "SCRAP info");
    if (p)
    {
        cur_input = p;
    }

    while (true)
    {
        char state_str[16] = {0};
        unsigned int block_number, hdrCrc, fwCrc;
        p = strchr(cur_input, '#');
        if (p == NULL)
        {
            // no more records found
            break;
        }

        int n = sscanf(p + 1, "%u %15s %x %x", &block_number, state_str, &hdrCrc, &fwCrc);
        if (n < 4)
        {
            continue; // ignore lines withot the form
        }

        // some healthy asserts
        assert(strlen(state_str) != 0);
        assert(hdrCrc != 0);
        assert(fwCrc != 0);

        if (block_number >= module.blocks.size())
        {
            // if number of blocks is differnt from host return immediatly and update all blocks
            LOG_DEBUG(LOG_TAG, "Block number(%zu) not found in host. Update all blocks", block_number);
            std::fill(rv.begin(), rv.end(), true);
            break;
        }

        auto host_block_crc = module.blocks[block_number].crc;
        // update if not ok or if one of the crcs differs from the others
        bool should_update = strcmp(state_str, "OK") != 0 || hdrCrc != fwCrc || hdrCrc != host_block_crc;
        rv[block_number] = should_update;

        LOG_DEBUG(LOG_TAG, "Block #%zu: fw: %s 0x%08x 0x%08x, local: 0x%08x, %s", block_number, state_str, hdrCrc,
                  fwCrc, host_block_crc, should_update ? "yes update" : "no update");

        cur_input = p + 9; // prepare to search next record (strchr(cur_input, '#'))
    }
    _comm->ConsumeScanned();
    return rv;
}

// parse 'dl' ack
bool FwUpdateEngine::ParseDlResponse(const std::string& name, size_t blkNo, size_t sz)
{
    char* logBuf = _comm->GetScanPtr();
    char str[64];
    ::snprintf(str, sizeof(str), "%s : blk %zu sz=%zu", name.c_str(), blkNo, sz);
    bool ack = strstr(logBuf, str) != NULL;

    if (!ack)
        LOG_DEBUG(LOG_TAG, "cannot find %s", str);

    _comm->ConsumeScanned();
    return ack;
}

// parse 'dl' send buffer result - return true if 'dl ret=0' is returned from device, false otherwise
bool FwUpdateEngine::ParseDlBlockResult()
{
    const char* dlRetStr = "dl ret=";
    char* logBuf = _comm->GetScanPtr();
    char* p = strstr(logBuf, dlRetStr);
    int result = -1;
    bool rv = false;
    if (p)
    {
        p += strlen(dlRetStr);
        uint32_t n = sscanf(p, "%d", &result);
        if (n != 1)
            rv = false;
        else
            rv = result == 0;
    }

    _comm->ConsumeScanned();
    return rv;
}

void FwUpdateEngine::BurnModule(ProgressTick tick, const ModuleInfo& module, const Buffer& buffer, bool is_first,
                                bool is_last, bool force_full)
{
    // send dlver command to get the module's state
    _comm->WriteCmd(Cmds::dlver());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ModuleVersionInfo version_info;
    bool success = ConsumeDlVerResponse(module.name, version_info);
    if (!success)
    {
        throw std::runtime_error("Failed parsing verinfo response");
    }
    // send dlinfo command to get the module's block info
    _comm->WriteCmd(Cmds::dlinfo(module.name));
    _comm->WaitForStr("dlinfo end", std::chrono::milliseconds {1000});
    auto block_update_list = FwUpdateEngine::GetBlockUpdateList(module, force_full);
    assert(module.blocks.size() == block_update_list.size());
    auto n_updates = std::count(block_update_list.begin(), block_update_list.end(), true);
    auto need_update = n_updates > 0;
    LOG_DEBUG(LOG_TAG, "Module %s: number of blocks to update: %zu", module.name.c_str(), n_updates);
    if (!need_update)
    {
        LOG_DEBUG(LOG_TAG, "Module %s: all CRC matched, no need to update", module.name.c_str());
    }

    // decide if module needs to be updated, if not - send 'fake' progress reports
    if (!need_update && version_info.state == ModuleVersionInfo::State::Active)
    {
        LOG_DEBUG(LOG_TAG, "No need to update module, skipping...");

        for (uint32_t i = 0; i < module.blocks.size(); ++i)
        {
            tick();
        }

        if (is_last)
        {
            // if this is the last module, we stop the reader thread
            _comm->StopReaderThread();

            // activate last module and reboot
            _comm->WriteCmd(Cmds::dlact(true), false);
        }

        return;
    }

    // allow partial updates only when current module is already mid-update (previously interrupted)
    if (version_info.state != ModuleVersionInfo::State::ActiveUpdating)
    {
        LOG_DEBUG(LOG_TAG, "Resetting block update list");
        std::fill(block_update_list.begin(), block_update_list.end(), true);
    }

    // send dlinit - if we're starting a session, open it
    _comm->WriteCmd(Cmds::dlinit(module.name, module.version, module.size, is_first, module.crc, BlockSize));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // send CRCs of all blocks to fw as binary array of [n x uin32_t] bytes (little endian)
    std::vector<uint32_t> blkCrc;
    for (const auto& block : module.blocks)
    {
        blkCrc.push_back(block.crc);
    }
    _comm->WriteBinary((const char*)blkCrc.data(), blkCrc.size() * sizeof(uint32_t));
    _comm->ConsumeScanned();

    LOG_DEBUG(LOG_TAG, "Starting module %s update", module.name.c_str());
    for (auto i = 0; i < module.blocks.size(); ++i)
    {
        bool should_update_block = block_update_list[i];
        if (!should_update_block)
        {
            LOG_DEBUG(LOG_TAG, "Module %s, block #%d already up-to-date, skipping...", module.name.c_str(), i);
            continue;
        }

        LOG_DEBUG(LOG_TAG, "Module %s, block #%d, updating...", module.name.c_str(), i);

        auto sz = module.blocks[i].size;
        const unsigned char* sendBuf = buffer.data() + module.blocks[i].offset;

        size_t sendSz = sz;

        _comm->WriteCmd(Cmds::dl(i));
        _comm->WaitForIdle();
        bool dlAck = ParseDlResponse(module.name, i, sz);
        if (!dlAck)
        {
            throw std::runtime_error("Did not receive 'dl ack'");
        }

        _comm->WriteBinary((char*)sendBuf, sendSz);

        auto timeoutMs = 2000 * BlockSize / (64 * 1024);
        _comm->WaitForStr("dl ret=", std::chrono::milliseconds {timeoutMs});
        auto dlErr = ParseDlBlockResult();
        if (!dlErr)
        {
            throw std::runtime_error("Error while parsing block");
        }

        tick();
    }

    // update finished - send dlver, receive response and check crcs
    _comm->WriteCmd(Cmds::dlinfo(module.name));
    _comm->WaitForStr("dlinfo end", std::chrono::milliseconds {3000});
    block_update_list = FwUpdateEngine::GetBlockUpdateList(module, false /* no force_full */);
    need_update = std::find(block_update_list.begin(), block_update_list.end(), true) != block_update_list.end();
    if (need_update)
    {
        throw std::runtime_error("Update failed");
    }

    if (is_last)
    {
        // if this is the last module, we stop the reader thread
        _comm->StopReaderThread();

        // activate last module and reboot
        _comm->WriteCmd(Cmds::dlact(true), false);
    }
    else
    {
        // activate the module
        _comm->WriteCmd(Cmds::dlact(false));

        // wait for validation string if not last module
        _comm->WaitForStr("validation ok", std::chrono::milliseconds {3000});
    }

    LOG_DEBUG(LOG_TAG, "update finished");
}

void FwUpdateEngine::BurnSelectModules(const ModuleVector& modules, ProgressTick tick, bool force_full)
{
    size_t module_count = 0;
    for (const auto& module : modules)
    {
        module_count++;

        auto buffer = LoadFileToBuffer(module.filename, module.aligned_size, module.size, module.file_offset);
        if (buffer.empty())
        {
            throw std::runtime_error("Failed loading firwmare file");
        }
        auto is_last_module = module_count == modules.size();
        bool is_first_module = module_count == 1;
        BurnModule(tick, module, buffer, is_first_module, is_last_module, force_full);

        LOG_INFO(LOG_TAG, "Module %s done", module.name.c_str());
    }
}

ModuleVector FwUpdateEngine::ModulesFromFile(const std::string& path)
{
    LOG_INFO(LOG_TAG, "Extract modules from \"%s\"", path.c_str());
    auto modules = ParseUfifToModules(path, BlockSize);
    // validate that we get known module names
    for (const auto& module : modules)
    {
        if (AllowedModules.find(module.name) == AllowedModules.end())
            throw std::runtime_error("Found invalid module name in file: " + module.name);
    }

    LOG_INFO(LOG_TAG, "Extracted %zu modules", modules.size());
    LOG_DEBUG(LOG_TAG, "");
    return modules;
}

void FwUpdateEngine::BurnModules(const Settings& settings, const ModuleVector& modules, ProgressCallback on_progress)
{
	if (modules.empty())
	{
  	    LOG_ERROR(LOG_TAG, "Received empty modules list");
		return;
	}

    // progress pre-processing
    size_t total_number_of_blocks = 0;

    for (const auto& module : modules)
        total_number_of_blocks += module.blocks.size();

    // calculate the effect each block has on the overall progress
    float progress_delta = 1.0f / total_number_of_blocks;

    float overall_progress = 0.0f;
    // wrap external progress callback with a "tick progress" lambda, called every time a block is sent.
    auto progress_tick = [progress_delta, on_progress, &overall_progress]() {
        overall_progress += progress_delta;
        on_progress(overall_progress);
    };

#ifdef ANDROID
    _comm = std::make_unique<FwUpdaterComm>(settings.android_config);
#else
    _comm = std::make_unique<FwUpdaterComm>(settings.port);
#endif
    try
    {
        _comm->WaitForIdle();
        _comm->WriteCmd(Cmds::dlspd(settings.baud_rate), true);
        _comm->WriteCmd(Cmds::dlver(), true);
        on_progress(0.0f);
        BurnSelectModules(modules, progress_tick, settings.force_full);
        on_progress(1.0f);
    }
    catch (const std::exception&)
    {
        // close connection if exists and rethrow
        _comm.reset();
        throw;
    }
}
} // namespace FwUpdate
} // namespace RealSenseID
