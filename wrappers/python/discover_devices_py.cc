// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include <cstdint>
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <RealSenseID/DiscoverDevices.h>
#include "rsid_py.h"

#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
namespace py = pybind11;


void init_discover_devices(pybind11::module& m)
{
    using namespace RealSenseID;
    m.def("discover_devices", [] {
        auto devices = RealSenseID::DiscoverDevices();
        // convert to list of strings
        std::vector<std::string> rv;
        std::transform(devices.begin(), devices.end(), std::back_inserter(rv),
                       [](const DeviceInfo& device_info) { return device_info.serialPort; });
        return rv;
    });

    m.def("discover_capture", &RealSenseID::DiscoverCapture);
}