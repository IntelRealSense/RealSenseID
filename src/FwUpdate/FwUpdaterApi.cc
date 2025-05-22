// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.


#include "RealSenseID/FwUpdater.h"
#include "F45x/FwUpdaterF45x.h"
#include "F46x/FwUpdaterF46x.h"
#include <stdexcept>

namespace RealSenseID
{

FwUpdater::FwUpdater(DeviceType deviceType)
{
    switch (deviceType)
    {
    case RealSenseID::DeviceType::F45x:
        _impl = new FwUpdateF45x::FwUpdaterF45x();
        break;
    case RealSenseID::DeviceType::F46x:
        _impl = new FwUpdateF46x::FwUpdaterF46x();
        break;
    default:
        throw std::invalid_argument("Unknown device type");
    }
}

bool FwUpdater::ExtractFwInformation(const char* binPath, std::string& outFwVersion, std::string& outRecognitionVersion,
                                     std::vector<std::string>& moduleNames) const
{
    return _impl->ExtractFwInformation(binPath, outFwVersion, outRecognitionVersion, moduleNames);
}


Status FwUpdater::UpdateModules(EventHandler* handler, Settings settings, const char* binPath) const
{
    return _impl->UpdateModules(handler, std::move(settings), binPath);
}

bool FwUpdater::IsSkuCompatible(const FwUpdater::Settings& settings, const char* binPath, int& expectedSkuVer, int& deviceSkuVer) const
{
    return _impl->IsSkuCompatible(settings, binPath, expectedSkuVer, deviceSkuVer);
}

} // namespace RealSenseID