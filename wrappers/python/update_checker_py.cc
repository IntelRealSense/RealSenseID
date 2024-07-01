// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2024 Intel Corporation. All Rights Reserved.

#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include "RealSenseID/UpdateChecker.h"
#include "rsid_py.h"
#include <string>

#include <sstream>
#include <iostream>
#include <utility>

namespace py = pybind11;
using namespace RealSenseID;


class ReleaseInfoPy {
public:
    uint64_t sw_version = 0;
    uint64_t fw_version = 0;
    std::string sw_version_str;
    std::string fw_version_str;
    std::string release_url;
    std::string release_notes_url;
};

class UpdateCheckPy
{
public:

    static ReleaseInfoPy release_to_py(const RealSenseID::UpdateCheck::ReleaseInfo& release_info) {
        ReleaseInfoPy remote_py {
            release_info.sw_version,
            release_info.fw_version,
            "", "", "", ""
        };
        if (release_info.sw_version_str) {
            remote_py.sw_version_str = std::string (release_info.sw_version_str);
        }
        if (release_info.fw_version_str) {
            remote_py.fw_version_str = std::string (release_info.fw_version_str);
        }
        if (release_info.release_url) {
            remote_py.release_url = std::string (release_info.release_url);
        }
        if (release_info.release_notes_url) {
            remote_py.release_notes_url = std::string (release_info.release_notes_url);
        }
        return remote_py;
    }

    static ReleaseInfoPy GetRemoteReleaseInfo()
    {
        RealSenseID::UpdateCheck::ReleaseInfo remote;
        auto updateChecker = std::make_shared<RealSenseID::UpdateCheck::UpdateChecker>();
        auto status = updateChecker->GetRemoteReleaseInfo(remote);
        if (status != RealSenseID::Status::Ok)
        {
            throw std::runtime_error(std::string("Failed to get remote release info with status: ") +
                                     RealSenseID::Description(status));
        }
        updateChecker.reset();
        return release_to_py(remote);
    }

    static ReleaseInfoPy GetLocalReleaseInfo(const std::string& port)
    {
        RealSenseID::UpdateCheck::ReleaseInfo local;
        auto updateChecker = std::make_shared<RealSenseID::UpdateCheck::UpdateChecker>();
        RealSenseID::SerialConfig config {port.c_str()};
        auto status = updateChecker->GetLocalReleaseInfo(config, local);
        if (status != RealSenseID::Status::Ok)
        {
            throw std::runtime_error(std::string("Failed to get local release info with status: ") +
                                     RealSenseID::Description(status));
        }
        updateChecker.reset();
        return release_to_py(local);
    }

    static std::tuple<bool,ReleaseInfoPy, ReleaseInfoPy> IsUpdateAvailable (const std::string& port) {
        auto remote = GetRemoteReleaseInfo();
        auto local = GetLocalReleaseInfo(port);

        bool is_update_available = remote.fw_version > local.fw_version &&
                                   remote.sw_version > local.sw_version;

        return std::make_tuple(is_update_available, local, remote);
    }

};


void init_update_checker(pybind11::module& m)
{
    py::class_<ReleaseInfoPy, std::shared_ptr<ReleaseInfoPy>>(m, "ReleaseInfo")
        .def(py::init<>())
        .def_readonly("sw_version", &ReleaseInfoPy::sw_version)
        .def_readonly("fw_version", &ReleaseInfoPy::fw_version)
        .def_readonly("sw_version_str", &ReleaseInfoPy::sw_version_str)
        .def_readonly("fw_version_str", &ReleaseInfoPy::fw_version_str)
        .def_readonly("release_url", &ReleaseInfoPy::release_url)
        .def_readonly("release_notes_url", &ReleaseInfoPy::release_notes_url)
        .def("__repr__", [](const ReleaseInfoPy& fp) {
            std::ostringstream oss;
            oss << "<rsid_py.ReleaseInfo "
                << "sw_version=" << fp.sw_version << ", "
                << "fw_version=" << fp.fw_version << ", "
                << "sw_version_str=" << fp.sw_version_str << ", "
                << "fw_version_str=" << fp.fw_version_str << ", "
                << "release_url=" << fp.release_url << ", "
                << "release_notes_url=" << fp.release_notes_url << ">";
            return oss.str();
        });

    py::class_<UpdateCheckPy, std::shared_ptr<UpdateCheckPy>>(m, "UpdateChecker")
        .def(py::init<>())
        .def_static("get_local_release_info", &UpdateCheckPy::GetLocalReleaseInfo,
            R"""(
            Get device & host release info.
            Parameters
            ----------
            port: str
                serial port for the device
            Returns
            ----------
            ReleaseInfo
                local_release_info
            )""",
            py::arg("port").none(false),
            py::call_guard<py::gil_scoped_release>())

        .def_static("get_remote_release_info", &UpdateCheckPy::GetRemoteReleaseInfo,
            R"""(
            Get remote/update release info.,
            Returns
            ----------
            ReleaseInfo
                remote_release_info
            )""",
            py::call_guard<py::gil_scoped_release>())

        .def_static("is_update_available", &UpdateCheckPy::IsUpdateAvailable,
            R"""(
            Check if update is available.
            Parameters
            ----------
            port: str
                serial port for the device
            Returns
            ----------
            tuple[bool, ReleaseInfo, ReleaseInfo]
                (is_update_available: bool, local_release_info: ReleaseInfo, remote_release_info: ReleaseInfo)
                Where the bool represents update available if True.
                First ReleaseInfo is the local device/host and second ReleaseInfo is the remote/server latest version
            )""",
            py::arg("port").none(false),
            py::call_guard<py::gil_scoped_release>());
}