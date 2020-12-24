// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseIDExports.h"

#include <string>

namespace RealSenseID
{
/**
 * Serial communication statuses.
 */
enum class RSID_API SerialStatus
{
    Ok = 100,     ///< Communication succeeded
    Error,        ///< Error communication on the serial line
    SecurityError ///< Error during secure session protocol
};


/**
 * Return c string description of the status
 * @param status to describe.
 */
RSID_API const char* Description(SerialStatus status);


/**
 * operator<< support for SerialStatus
 */

template <typename OStream>
inline OStream& operator<<(OStream& os, const SerialStatus& status)
{
    os << Description(status);
    return os;
}
} // namespace RealSenseID
