// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/Status.h"
#include "PacketManager/CommonTypes.h"
#include "RealSenseID/EnrollStatus.h"
#include "RealSenseID/AuthenticateStatus.h"
#include "RealSenseID/FacePose.h"
#include "RealSenseID/DeviceConfig.h"

namespace RealSenseID
{
Status ToStatus(PacketManager::SerialStatus serial_status);
EnrollStatus ToEnrollStatus(PacketManager::SerialStatus serial_status);
AuthenticateStatus ToAuthStatus(PacketManager::SerialStatus serial_status);
PacketManager::SerialStatus ToSerialStatus(Status fa_status);
} // namespace RealSenseID
