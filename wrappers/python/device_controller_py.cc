// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include <cstdint>
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <RealSenseID/DeviceController.h>
#include "rsid_py.h"
#include <string>

namespace py = pybind11;

void init_device_controller(pybind11::module& m)
{
    using namespace RealSenseID;
    py::class_<DeviceController>(m, "DeviceController")
        .def(py::init<>())                          // default ctor
        .def(py::init([](const std::string& port) { // ctor with port to connect
            auto f = std::make_unique<DeviceController>();
            RSID_THROW_ON_ERROR(f->Connect(SerialConfig {port.c_str()}));
            return f;
        }))

        .def("__enter__", [](DeviceController& self) { return &self; })
        .def("__exit__", [](DeviceController& self, void*, void*, void*) { self.Disconnect(); })

        .def(
            "connect",
            [](DeviceController& self, const std::string& port) {
                RSID_THROW_ON_ERROR(self.Connect(SerialConfig {port.c_str()}));
            },
            py::call_guard<py::gil_scoped_release>())

        .def("disconnect", &DeviceController::Disconnect, py::call_guard<py::gil_scoped_release>())
        .def("reboot", &DeviceController::Reboot, py::call_guard<py::gil_scoped_release>())
        .def(
            "query_firmware_version",
            [](DeviceController& self) {
                std::string version;
                RSID_THROW_ON_ERROR(self.QueryFirmwareVersion(version));
                return version;
            },
            py::call_guard<py::gil_scoped_release>())

        .def(
            "query_serial_number",
            [](DeviceController& self) {
                std::string serial;
                RSID_THROW_ON_ERROR(self.QuerySerialNumber(serial));
                return serial;
            },
            py::call_guard<py::gil_scoped_release>())
        .def("ping", &DeviceController::Ping, py::call_guard<py::gil_scoped_release>());
}