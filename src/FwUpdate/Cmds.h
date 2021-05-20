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
    /**
     * retrieves information for all modules
     *
     * returns a formatted dlver command
     */
    std::string dlver();

    /**
     * controls baud rate for the firmware update process
     *
     * spd - baud rate for firmware upgrade
     *
     * returns a formatted dlspd command
     */
    std::string dlspd(uint32_t spd);

    /**
     * retrieves specific module extended information
     *
     * module_name - name of requested module
     *
     * returns a formatted dlinfo command
     */
    std::string dlinfo(const std::string& module_name);

    /**
     * starts a module update process
     *
     * module_name      - updated module name
     * version          - updated module version
     * size             - updated module size
     * start_session    - start a new multiple-module update session
     * crc              - checksum calculated on the entire module
     * block_size       - updated module block size
     *
     * returns a formatted dlinit command
     */
    std::string dlinit(const std::string& name, const std::string& version, size_t size, bool start_session,
                              uint32_t crc, uint32_t block_size);

    /**
     * update specific block number of updated module
     *
     * n - block number to update
     *
     * returns a formatted dl command
     */
    std::string dl(size_t n);
  
    /**
     * finishes a module update process, if it was valid
     *
     * is_last - end a multiple-module update session
     *
     * returns a formatted dlact command
     */
    std::string dlact(bool is_last);
    };
} // namespace FwUpdate
} // namespace RealSenseID