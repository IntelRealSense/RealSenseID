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

Status DeviceController::Connect(const SerialConfig& config)
{
    return _impl->Connect(config);
}

#ifdef ANDROID
Status DeviceController::Connect(int fileDescriptor, int readEndpointAddress, int writeEndpointAddress)
{
    return _impl->Connect(fileDescriptor, readEndpointAddress, writeEndpointAddress);
}
#endif

void DeviceController::Disconnect()
{
    _impl->Disconnect();
}

bool DeviceController::Reboot()
{
    return _impl->Reboot();
}

Status DeviceController::QueryFirmwareVersion(std::string& version)
{
    return _impl->QueryFirmwareVersion(version);
}

Status DeviceController::QuerySerialNumber(std::string& serial)
{
    return _impl->QuerySerialNumber(serial);
}

Status DeviceController::Ping()
{
    return _impl->Ping();
}

} // namespace RealSenseID