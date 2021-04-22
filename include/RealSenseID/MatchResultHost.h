// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#pragma once

namespace RealSenseID 
{

using match_calc_t = short; 

 /**
 * Result used by the Matcher module.
 */
struct MatchResultHost
{
    bool success = false;
    bool should_update = false;
    match_calc_t confidence = 0;
    match_calc_t score = 0;
};
} // namespace RealSenseID