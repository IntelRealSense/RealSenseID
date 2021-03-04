// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#pragma once

#include <string>

namespace RealSenseID
{
namespace FwUpdate
{
namespace Cmds
{
    /****************** dlver cmd ******************/
    std::string dlver();

    /****************** dlspd cmd ******************/
    // dlspd should be the 1st if exists
    std::string dlspd(uint32_t spd);

    /****************** dlinfo cmd ******************/
    // show info of specified module
    // dlinfo $module
    std::string dlinfo(const std::string& module_name);

    /****************** dlinit cmd ******************/
    // dlinit initialize uisp header to prepare to update specified module
    // dlinit FW ver=d.e.f.g sz=24340250 blksz=512KB crc=aabbccdd
    std::string dlinit(const std::string& name, const std::string& version, size_t size, bool start_session,
                              uint32_t crc, uint32_t block_size);

    
    /****************** dl cmd ******************/
    // dl update specified block of inited module (specified with dlinit)
    // dl $blk#
    std::string dl(size_t n);
  
    /****************** dlact cmd ******************/
    //std::string dlact(bool end_session, bool reboot);
    std::string dlact(bool is_last);
    };
} // namespace FwUpdate
} // namespace RealSenseID