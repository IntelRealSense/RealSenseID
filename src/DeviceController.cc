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

Status DeviceController::QueryOtpVersion(uint8_t& version)
{
    return _impl->QueryOtpVersion(version);
}

Status DeviceController::Ping()
{
    return _impl->Ping();
}

Status DeviceController::FetchLog(std::string& log)
{
    return _impl->FetchLog(log);
}

Status DeviceController::GetColorGains(int& red, int& blue)
{
    return _impl->GetColorGains(red, blue);
}

Status DeviceController::SetColorGains(int red, int blue)
{
    return _impl->SetColorGains(red, blue);
}

} // namespace RealSenseID