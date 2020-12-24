// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/SerialStatus.h"
#include "RealSenseID/AuthenticateStatus.h"
#include "RealSenseID/EnrollStatus.h"
#include "RealSenseID/FacePose.h"
#include "PacketManager/CommonTypes.h"

namespace RealSenseID
{
SerialStatus ToSerialStatus(PacketManager::Status serial_status);
EnrollStatus ToEnrollStatus(PacketManager::Status serial_status);
AuthenticateStatus ToAuthStatus(PacketManager::Status serial_status);
} // namespace RealSenseID
