// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/DeviceController.h"
#include "DeviceControllerImpl.h"

namespace RealSenseID
{
DeviceController::DeviceController() : _impl {new DeviceControllerImpl()}
{
}

DeviceController::~DeviceController()
{
    try
    {
        delete _impl;
    }
    catch (...)
    {
    }
}

SerialStatus DeviceController::Connect(const SerialConfig& config)
{
    return _impl->Connect(config);
}

void DeviceController::Disconnect()
{
    _impl->Disconnect();
}

bool DeviceController::Reboot()
{
    return _impl->Reboot();
}
} // namespace RealSenseID