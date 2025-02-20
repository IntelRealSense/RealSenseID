// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include <RealSenseID/FaceAuthenticator.h>
#include <RealSenseID/Version.h>
#include <RealSenseID/FaceprintsDefines.h>

#include "rsid_py.h"

#include <iostream>
#include <sstream>
#include <string>

namespace py = pybind11;
using namespace RealSenseID;

////////////////////////////////////////////////////////////////////////////////
// Define rsid python module
////////////////////////////////////////////////////////////////////////////////
static const char* version_str = RealSenseID::Version();
static const char* compatible_fw_str = RealSenseID::CompatibleFirmwareVersion();


PYBIND11_MODULE(rsid_py, m)
{
    m.doc() = R"pbdoc(
        RealSenseID Python Bindings
        ==============================
        Library for accessing Intel RealSenseID cameras
    )pbdoc";

    m.attr("__version__") = version_str;
    m.attr("compatible_firmware") = compatible_fw_str;
    m.attr("faceprints_version") = RSID_FACEPRINTS_VERSION;

    init_face_authenticator(m);
    init_device_controller(m);
    init_discover_devices(m);
    init_update_checker(m);
    init_fw_updater(m);
    init_logging(m);

#ifdef RSID_PREVIEW
    init_preview(m);
    ;
#endif // RSID_PREVIEW
}
