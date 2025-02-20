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
enum class RSID_API Status
{
    Ok = 100,        /** Operation succeeded */
    Error,           /** Operation failed */
    SerialError,     /** Error communication on the serial line */
    SecurityError,   /** Error during secure session protocol */
    VersionMismatch, /** Version mismatch between host and device */
    CrcError,        /** CRC error indicates packet corruption */
    LicenseError,    /** License verification error. The requested operation is not supported by the license in the device */
    LicenseCheck,    /** License check required. Need to provide license to device using ProvideLicense() API call */
    TooManySpoofs    /** Too many consecutive spoof attrmpts. Need to call unlock() API call */
};

/**
 * Return c string description of the status
 *
 * @param status to describe.
 */
RSID_API const char* Description(Status status);

template <typename OStream>
inline OStream& operator<<(OStream& os, const Status& status)
{
    os << Description(status);
    return os;
}
} // namespace RealSenseID
