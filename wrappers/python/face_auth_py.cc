// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include <RealSenseID/Version.h>
#include <RealSenseID/FaceAuthenticator.h>
#include <RealSenseID/EnrollmentCallback.h>
#include "rsid_py.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <sstream>
#include <iterator>
#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <type_traits>

namespace py = pybind11;
using namespace RealSenseID;

///////////////////////////////////////////////////////////////////////////////////
// Enroll Callback support
///////////////////////////////////////////////////////////////////////////////////
using EnrollStatusClbkFun = std::function<void(const EnrollStatus)>;
using EnrollHintClbkFun = EnrollStatusClbkFun;
using EnrollProgressClbkFun = std::function<void(const FacePose)>;
using Faces = std::vector<FaceRect>;
using FaceDetectedClbkFun = std::function<void(const Faces&, const unsigned int)>;
using LicenseStartClbkFun = std::function<void()>;
using LicenseEndClbkFun = std::function<void(RealSenseID::Status)>;

class EnrollCallbackPy : public EnrollmentCallback
{
    EnrollStatusClbkFun& _result_clbk;
    EnrollProgressClbkFun& _progress_clbk;
    EnrollHintClbkFun& _hint_clbk;
    FaceDetectedClbkFun& _face_clbk;

public:
    EnrollCallbackPy(EnrollStatusClbkFun& result_clbk, EnrollProgressClbkFun& progress_clbk,
                     EnrollHintClbkFun& hint_clbk, FaceDetectedClbkFun& facs_clbk) :
        _result_clbk {result_clbk},
        _progress_clbk {progress_clbk}, _hint_clbk {hint_clbk}, _face_clbk {facs_clbk}
    {
    }

    void OnResult(const EnrollStatus status) override
    {
        if (_result_clbk)
        {
            py::gil_scoped_acquire acquire;
            _result_clbk(status);
        }
    }

    void OnProgress(const FacePose pose) override
    {
        if (_progress_clbk)
        {
            py::gil_scoped_acquire acquire;
            _progress_clbk(pose);
        }
    }


    void OnHint(const EnrollStatus hint) override
    {
        if (_hint_clbk)
        {
            py::gil_scoped_acquire acquire;
            _hint_clbk(hint);
        }
    }

    void OnFaceDetected(const std::vector<FaceRect>& faces, const unsigned int ts) override
    {
        if (_face_clbk)
        {
            py::gil_scoped_acquire acquire;
            _face_clbk(faces, ts);
        }
    }
};


///////////////////////////////////////////////////////////////////////////////////
// Authenticate Callback support
///////////////////////////////////////////////////////////////////////////////////
using AuthStatusClbkFun = std::function<void(const AuthenticateStatus, const char* user_id)>;
using AuthHintClbkFun = std::function<void(const AuthenticateStatus)>;

class AuthCallbackPy : public AuthenticationCallback
{
    AuthStatusClbkFun& _result_clbk;
    AuthHintClbkFun& _hint_clbk;
    FaceDetectedClbkFun& _face_clbk;

public:
    AuthCallbackPy(AuthStatusClbkFun& result_clbk, AuthHintClbkFun& hint_clbk, FaceDetectedClbkFun& facs_clbk) :
        _result_clbk {result_clbk}, _hint_clbk {hint_clbk}, _face_clbk {facs_clbk}
    {
    }

    void OnResult(const AuthenticateStatus status, const char* user_id) override
    {
        if (_result_clbk)
        {
            py::gil_scoped_acquire acquire;
            _result_clbk(status, user_id);
        }
    }


    void OnHint(const AuthenticateStatus hint) override
    {
        if (_hint_clbk)
        {
            py::gil_scoped_acquire acquire;
            _hint_clbk(hint);
        }
    }

    void OnFaceDetected(const std::vector<FaceRect>& faces, const unsigned int ts) override
    {
        if (_face_clbk)
        {
            py::gil_scoped_acquire acquire;
            _face_clbk(faces, ts);
        }
    }
};


///////////////////////////////////////////////////////////////////////////////////
// Faceprints Enroll Callback support
///////////////////////////////////////////////////////////////////////////////////

using FaceprintsEnrollStatusClbkFun =
    std::function<void(const EnrollStatus, const ExtractedFaceprintsElement* faceprints)>;


class FaceprintsEnrollCallbackPy : public EnrollFaceprintsExtractionCallback
{
    FaceprintsEnrollStatusClbkFun& _result_clbk;
    EnrollProgressClbkFun& _progress_clbk;
    EnrollHintClbkFun& _hint_clbk;
    FaceDetectedClbkFun& _face_clbk;

public:
    FaceprintsEnrollCallbackPy(FaceprintsEnrollStatusClbkFun& result_clbk, EnrollProgressClbkFun& progress_clbk,
                               EnrollHintClbkFun& hint_clbk, FaceDetectedClbkFun& facs_clbk) :
        _result_clbk {result_clbk},
        _progress_clbk {progress_clbk}, _hint_clbk {hint_clbk}, _face_clbk {facs_clbk}
    {
    }

    void OnResult(const EnrollStatus status, const ExtractedFaceprints* faceprints) override
    {
        if (_result_clbk)
        {
            py::gil_scoped_acquire acquire;
            if (faceprints != nullptr)
                _result_clbk(status, &faceprints->data);
            else
                _result_clbk(status, nullptr);
        }
    }

    void OnProgress(const FacePose pose) override
    {
        if (_progress_clbk)
        {
            py::gil_scoped_acquire acquire;
            _progress_clbk(pose);
        }
    }


    void OnHint(const EnrollStatus hint) override
    {
        if (_hint_clbk)
        {
            py::gil_scoped_acquire acquire;
            _hint_clbk(hint);
        }
    }

    void OnFaceDetected(const std::vector<FaceRect>& faces, const unsigned int ts) override
    {
        if (_face_clbk)
        {
            py::gil_scoped_acquire acquire;
            _face_clbk(faces, ts);
        }
    }
};
///////////////////////////////////////////////////////////////////////////////////
// Faceprints Auth Callback support
///////////////////////////////////////////////////////////////////////////////////
using FaceprintsAuthStatusClbkFun =
    std::function<void(const AuthenticateStatus, const ExtractedFaceprintsElement* faceprints)>;


class FaceprintsAuthCallbackPy : public AuthFaceprintsExtractionCallback
{
    FaceprintsAuthStatusClbkFun& _result_clbk;
    AuthHintClbkFun& _hint_clbk;
    FaceDetectedClbkFun& _face_clbk;

public:
    FaceprintsAuthCallbackPy(FaceprintsAuthStatusClbkFun& result_clbk, AuthHintClbkFun& hint_clbk,
                             FaceDetectedClbkFun& facs_clbk) :
        _result_clbk {result_clbk},
        _hint_clbk {hint_clbk}, _face_clbk {facs_clbk}
    {
    }

    void OnResult(const AuthenticateStatus status, const ExtractedFaceprints* faceprints) override
    {
        if (_result_clbk)
        {
            py::gil_scoped_acquire acquire;
            if (faceprints != nullptr)
                _result_clbk(status, &faceprints->data);
            else
                _result_clbk(status, nullptr);
        }
    }


    void OnHint(const AuthenticateStatus hint) override
    {
        if (_hint_clbk)
        {
            py::gil_scoped_acquire acquire;
            _hint_clbk(hint);
        }
    }

    void OnFaceDetected(const std::vector<FaceRect>& faces, const unsigned int ts) override
    {
        if (_face_clbk)
        {
            py::gil_scoped_acquire acquire;
            _face_clbk(faces, ts);
        }
    }
};


// query users and return as std::vector
static std::vector<std::string> query_users(FaceAuthenticator& authenticator)
{
    std::vector<std::string> rv;
    unsigned int number_of_users = 0;
    auto status = authenticator.QueryNumberOfUsers(number_of_users);
    if (status != RealSenseID::Status::Ok)
    {
        throw std::runtime_error(std::string("QueryNumberOfUsers: ") + Description(status));
    }

    if (number_of_users == 0)
    {
        return rv;
    }

    // allocate needed arrays of user ids
    std::vector<std::array<char, RealSenseID::MAX_USERID_LENGTH>> arrays {number_of_users};

    std::vector<char*> ptrs;
    for (auto& ptr : arrays)
    {
        ptrs.push_back(ptr.data());
    }
    unsigned int nusers_in_out = number_of_users;
    status = authenticator.QueryUserIds(ptrs.data(), nusers_in_out);
    if (status != RealSenseID::Status::Ok)
    {
        throw std::runtime_error(std::string("QueryUserIds: ") + Description(status));
    }

    // convert to vector of strings
    rv.reserve(nusers_in_out);
    for (unsigned int i = 0; i < nusers_in_out; i++)
    {
        ptrs[i][RealSenseID::MAX_USERID_LENGTH - 1] = '\0';
        rv.push_back(std::string(ptrs[i]));
    }
    return rv;
}


std::ostream& operator<<(std::ostream& oss, const feature_t features[RSID_FEATURES_VECTOR_ALLOC_SIZE])
{
    oss << '[';
    for (size_t i = 0; i < RSID_FEATURES_VECTOR_ALLOC_SIZE; i++)
    {
        oss << features[i];
        if (i < RSID_FEATURES_VECTOR_ALLOC_SIZE - 1)
            oss << ", ";
    }
    oss << ']';
    return oss;
}

void init_face_authenticator(pybind11::module& m)
{
    m.attr("RSID_FACEPRINTS_VERSION") =  RSID_FACEPRINTS_VERSION;
    m.attr("RSID_FEATURES_VECTOR_ALLOC_SIZE") =  RSID_FEATURES_VECTOR_ALLOC_SIZE;
    m.attr("RSID_NUM_OF_RECOGNITION_FEATURES") =  RSID_NUM_OF_RECOGNITION_FEATURES;

    py::enum_<Status>(m, "Status")
        .value("Ok", Status::Ok)
        .value("Error", Status::Error)
        .value("SerialError", Status::SerialError)
        .value("SecurityError", Status::SecurityError)
        .value("VersionMismatch", Status::VersionMismatch)
        .value("CrcError", Status::CrcError)
        .value("LicenseError", Status::LicenseError)
        .value("LicenseCheck", Status::LicenseCheck)
        .value("TooManySpoofs", Status::TooManySpoofs);

    py::enum_<AuthenticateStatus>(m, "AuthenticateStatus")
        .value("Success", AuthenticateStatus::Success)
        .value("NoFaceDetected", AuthenticateStatus::NoFaceDetected)
        .value("FaceDetected", AuthenticateStatus::FaceDetected)
        .value("LedFlowSuccess", AuthenticateStatus::LedFlowSuccess)
        .value("FaceIsTooFarToTheTop", AuthenticateStatus::FaceIsTooFarToTheTop)
        .value("FaceIsTooFarToTheBottom", AuthenticateStatus::FaceIsTooFarToTheBottom)
        .value("FaceIsTooFarToTheRight", AuthenticateStatus::FaceIsTooFarToTheRight)
        .value("FaceIsTooFarToTheLeft", AuthenticateStatus::FaceIsTooFarToTheLeft)
        .value("FaceTiltIsTooUp", AuthenticateStatus::FaceTiltIsTooUp)
        .value("FaceTiltIsTooDown", AuthenticateStatus::FaceTiltIsTooDown)
        .value("FaceTiltIsTooRight", AuthenticateStatus::FaceTiltIsTooRight)
        .value("FaceTiltIsTooLeft", AuthenticateStatus::FaceTiltIsTooLeft)
        .value("CameraStarted", AuthenticateStatus::CameraStarted)
        .value("CameraStopped", AuthenticateStatus::CameraStopped)
        .value("MaskDetectedInHighSecurity", AuthenticateStatus::MaskDetectedInHighSecurity)
        .value("Spoof", AuthenticateStatus::Spoof)
        .value("Forbidden", AuthenticateStatus::Forbidden)
        .value("DeviceError", AuthenticateStatus::DeviceError)
        .value("Failure", AuthenticateStatus::Failure)
        .value("TooManySpoofs", AuthenticateStatus::TooManySpoofs)
        .value("InvalidFeatures", AuthenticateStatus::InvalidFeatures)        
        .value("Ok", AuthenticateStatus::Ok)
        .value("Error", AuthenticateStatus::Error)
        .value("SerialError", AuthenticateStatus::SerialError)
        .value("SecurityError", AuthenticateStatus::SecurityError)
        .value("VersionMismatch", AuthenticateStatus::VersionMismatch)
        .value("CrcError", AuthenticateStatus::CrcError)
        .value("LicenseError", AuthenticateStatus::LicenseError)
        .value("LicenseCheck", AuthenticateStatus::LicenseCheck)
        .value("Spoof_2D", AuthenticateStatus::Spoof_2D)
        .value("Spoof_3D", AuthenticateStatus::Spoof_3D)
        .value("Spoof_LR", AuthenticateStatus::Spoof_LR)
        .value("Spoof_Surface", AuthenticateStatus::Spoof_Surface)
        .value("Spoof_Disparity", AuthenticateStatus::Spoof_Disparity)
        .value("Spoof_Plane_Disparity", AuthenticateStatus::Spoof_Plane_Disparity);

    py::enum_<EnrollStatus>(m, "EnrollStatus")
        .value("Success", EnrollStatus::Success)
        .value("NoFaceDetected", EnrollStatus::NoFaceDetected)
        .value("FaceDetected", EnrollStatus::FaceDetected)
        .value("LedFlowSuccess", EnrollStatus::LedFlowSuccess)
        .value("FaceIsTooFarToTheTop", EnrollStatus::FaceIsTooFarToTheTop)
        .value("FaceIsTooFarToTheBottom", EnrollStatus::FaceIsTooFarToTheBottom)
        .value("FaceIsTooFarToTheRight", EnrollStatus::FaceIsTooFarToTheRight)
        .value("FaceIsTooFarToTheLeft", EnrollStatus::FaceIsTooFarToTheLeft)
        .value("FaceTiltIsTooUp", EnrollStatus::FaceTiltIsTooUp)
        .value("FaceTiltIsTooDown", EnrollStatus::FaceTiltIsTooDown)
        .value("FaceTiltIsTooRight", EnrollStatus::FaceTiltIsTooRight)
        .value("FaceTiltIsTooLeft", EnrollStatus::FaceTiltIsTooLeft)
        .value("FaceIsNotFrontal", EnrollStatus::FaceIsNotFrontal)
        .value("CameraStarted", EnrollStatus::CameraStarted)
        .value("CameraStopped", EnrollStatus::CameraStopped)
        .value("MultipleFacesDetected", EnrollStatus::MultipleFacesDetected)
        .value("Failure", EnrollStatus::Failure)
        .value("DeviceError", EnrollStatus::DeviceError)
        .value("EnrollWithMaskIsForbidden", EnrollStatus::EnrollWithMaskIsForbidden)
        .value("Spoof", EnrollStatus::Spoof)
        .value("InvalidFeatures", EnrollStatus::InvalidFeatures)        
        .value("Ok", EnrollStatus::Ok)
        .value("Error", EnrollStatus::Error)
        .value("SerialError", EnrollStatus::SerialError)
        .value("SecurityError", EnrollStatus::SecurityError)
        .value("VersionMismatch", EnrollStatus::VersionMismatch)
        .value("CrcError", EnrollStatus::CrcError)
        .value("LicenseError", EnrollStatus::LicenseError)
        .value("LicenseCheck", EnrollStatus::LicenseCheck)
        .value("Spoof_2D", EnrollStatus::Spoof_2D)
        .value("Spoof_3D", EnrollStatus::Spoof_3D)
        .value("Spoof_LR", EnrollStatus::Spoof_LR)
        .value("Spoof_Surface", EnrollStatus::Spoof_Surface)
        .value("Spoof_Disparity", EnrollStatus::Spoof_Disparity)
        .value("Spoof_Plane_Disparity", EnrollStatus::Spoof_Plane_Disparity);


    py::enum_<FacePose>(m, "FacePose")
        .value("Center", FacePose::Center)
        .value("Up", FacePose::Up)
        .value("Down", FacePose::Down)
        .value("Left", FacePose::Left)
        .value("Right", FacePose::Right);

    py::class_<FaceRect>(m, "FaceRect")
        .def("__copy__",  [](const FaceRect &self) {
            return FaceRect(self);
        })
        .def(py::init<>())
        .def_readwrite("x", &FaceRect::x)
        .def_readwrite("y", &FaceRect::y)
        .def_readwrite("w", &FaceRect::w)
        .def_readwrite("h", &FaceRect::h)
        .def("__repr__", [](const FaceRect& f) {
            std::ostringstream oss;
            oss << "<rsid_py.FaceRect " << f.x << ',' << f.y << ' ' << f.w << 'x' << f.y << '>';
            return oss.str();
        });

    py::enum_<DeviceConfig::CameraRotation>(m, "CameraRotation")
        .value("Rotation_0_Deg", DeviceConfig::CameraRotation::Rotation_0_Deg)
        .value("Rotation_180_Deg", DeviceConfig::CameraRotation::Rotation_180_Deg)
        .value("Rotation_90_Deg", DeviceConfig::CameraRotation::Rotation_90_Deg)
        .value("Rotation_270_Deg", DeviceConfig::CameraRotation::Rotation_270_Deg);

    py::enum_<DeviceConfig::SecurityLevel>(m, "SecurityLevel")
        .value("High", DeviceConfig::SecurityLevel::High)
        .value("Medium", DeviceConfig::SecurityLevel::Medium)
        .value("Low", DeviceConfig::SecurityLevel::Low);


    py::enum_<DeviceConfig::MatcherConfidenceLevel>(m, "MatcherConfidenceLevel")
        .value("High", DeviceConfig::MatcherConfidenceLevel::High)
        .value("Medium", DeviceConfig::MatcherConfidenceLevel::Medium)
        .value("Low", DeviceConfig::MatcherConfidenceLevel::Low);

    py::enum_<DeviceConfig::AlgoFlow>(m, "AlgoFlow")
        .value("All", DeviceConfig::AlgoFlow::All)
        .value("FaceDetectionOnly", DeviceConfig::AlgoFlow::FaceDetectionOnly)
        .value("SpoofOnly", DeviceConfig::AlgoFlow::SpoofOnly)
        .value("RecognitionOnly", DeviceConfig::AlgoFlow::RecognitionOnly);


    py::enum_<DeviceConfig::FaceSelectionPolicy>(m, "FaceSelectionPolicy")
        .value("Single", DeviceConfig::FaceSelectionPolicy::Single)
        .value("All", DeviceConfig::FaceSelectionPolicy::All);

    py::enum_<DeviceConfig::DumpMode>(m, "DumpMode")
        .value("Disable", DeviceConfig::DumpMode::None)
        .value("CroppedFace", DeviceConfig::DumpMode::CroppedFace)
        .value("FullFrame", DeviceConfig::DumpMode::FullFrame);

    py::class_<DeviceConfig>(m, "DeviceConfig")
        .def(py::init<>())
        .def("__copy__",  [](const DeviceConfig &self) {
            return DeviceConfig(self);
        })
        .def_readwrite("camera_rotation", &DeviceConfig::camera_rotation)
        .def_readwrite("security_level", &DeviceConfig::security_level)
        .def_readwrite("algo_flow", &DeviceConfig::algo_flow)
        .def_readwrite("face_selection_policy", &DeviceConfig::face_selection_policy)
        .def_readwrite("dump_mode", &DeviceConfig::dump_mode)
        .def_readwrite("matcher_confidence_level", &DeviceConfig::matcher_confidence_level)
        .def_readwrite("max_spoofs", &DeviceConfig::max_spoofs)

        .def("__repr__", [](const DeviceConfig& cfg) {
            std::ostringstream oss;
            oss << "<rsid_py.DeviceConfig "
                << "camera_rotation=" << cfg.camera_rotation << ", "
                << "security_level=" << cfg.security_level << ", "
                << "matcher_confidence_level=" << cfg.matcher_confidence_level << ", "
                << "algo_flow=" << cfg.algo_flow << ", "
                << "face_selection_policy=" << cfg.face_selection_policy << ", "
                << "dump_mode=" << cfg.dump_mode << ", "
                << "max_spoofs=" << static_cast<unsigned>(cfg.max_spoofs) << '>';
            return oss.str();
        });

    py::enum_<FaceprintsType>(m, "FaceprintsType").value("W10", FaceprintsType::W10).value("RGB", FaceprintsType::RGB);

    py::class_<DBFaceprintsElement>(m, "Faceprints")
        .def(py::init<>())
        .def("__copy__",  [](const DBFaceprintsElement &self) {
            return DBFaceprintsElement(self);
        })
        .def_readwrite("version", &DBFaceprintsElement::version)
        .def_readwrite("features_type", &DBFaceprintsElement::featuresType)
        .def_readwrite("flags", &DBFaceprintsElement::flags)
        /* adaptiveDescriptorWithoutMask getter/getter */
        .def_property(
            "adaptive_descriptor_nomask",
            [](DBFaceprintsElement& self) {
                // getter - return as vector of feature_t
                return std::vector<feature_t> {std::begin(self.adaptiveDescriptorWithoutMask),
                                               std::end(self.adaptiveDescriptorWithoutMask)};
            },
            // setter of avg descriptor list
            [](DBFaceprintsElement& self, const std::vector<feature_t>& new_descriptors) {
                constexpr size_t n_elements = std::extent<decltype(self.adaptiveDescriptorWithoutMask)>::value;
                if (new_descriptors.size() != n_elements)
                {
                    throw std::runtime_error("Invalid number of elements in setter. Expected " +
                                             std::to_string(n_elements));
                }
                std::copy(new_descriptors.begin(), new_descriptors.end(), self.adaptiveDescriptorWithoutMask);
            })

        /* adaptiveDescriptorWitMask getter/getter */
        .def_property(
            "adaptive_descriptor_withmask",
            [](DBFaceprintsElement& self) {
                // getter - return as vector of feature_t
                PyErr_WarnEx(PyExc_DeprecationWarning,
                             "adaptive_descriptor_withmask is deprecated in C++ SDK.",
                             1);
                return std::vector<feature_t> {std::begin(self.adaptiveDescriptorWithMask),
                                               std::end(self.adaptiveDescriptorWithMask)};
            },
            // setter of avg descriptor list
            [](DBFaceprintsElement& self, const std::vector<feature_t>& new_descriptors) {
                constexpr size_t n_elements = std::extent<decltype(self.adaptiveDescriptorWithMask)>::value;
                static_assert(n_elements == RSID_FEATURES_VECTOR_ALLOC_SIZE,
                              "n_elements!=RSID_FEATURES_VECTOR_ALLOC_SIZE");
                PyErr_WarnEx(PyExc_DeprecationWarning,
                             "adaptive_descriptor_withmask is deprecated in C++ SDK.",
                             1);
                if (new_descriptors.size() != n_elements)
                {
                    throw std::runtime_error("Invalid number of elements in setter. Expected " +
                                             std::to_string(n_elements));
                }
                std::copy(new_descriptors.begin(), new_descriptors.end(), self.adaptiveDescriptorWithMask);
            })

        /* adaptiveDescriptorWitMask getter/getter */
        .def_property(
            "enroll_descriptor",
            [](DBFaceprintsElement& self) {
                // getter - return as vector of feature_t
                return std::vector<feature_t> {std::begin(self.enrollmentDescriptor),
                                               std::end(self.enrollmentDescriptor)};
            },
            // setter of avg descriptor list
            [](DBFaceprintsElement& self, const std::vector<feature_t>& new_descriptors) {
                constexpr size_t n_elements = std::extent<decltype(self.enrollmentDescriptor)>::value;
                static_assert(n_elements == RSID_FEATURES_VECTOR_ALLOC_SIZE,
                              "n_elements!=RSID_FEATURES_VECTOR_ALLOC_SIZE");
                if (new_descriptors.size() != n_elements)
                {
                    throw std::runtime_error("Invalid number of elements in setter. Expected " +
                                             std::to_string(n_elements));
                }
                std::copy(new_descriptors.begin(), new_descriptors.end(), self.enrollmentDescriptor);
            })

        /* reserved array getter/getter */
        .def_property(
            "reserved",
            [](DBFaceprintsElement& self) {
                // getter - return as vector of feature_t
                return std::vector<int> {std::begin(self.reserved), std::end(self.reserved)};
            },
            // setter of avg descriptor list
            [](DBFaceprintsElement& self, const std::vector<feature_t>& new_descriptors) {
                constexpr size_t n_elements = std::extent<decltype(self.reserved)>::value;
                if (new_descriptors.size() != n_elements)
                {
                    throw std::runtime_error("Invalid number of elements in setter. Expected " +
                                             std::to_string(n_elements));
                }
                std::copy(new_descriptors.begin(), new_descriptors.end(), self.reserved);
            })
        .def("__repr__", [](const DBFaceprintsElement& fp) {
            std::ostringstream oss;
            oss << "<rsid_py.Faceprints "
                << "version=" << fp.version << ", "
                << "feature_type=" << fp.featuresType << ", "
                << "flags=" << fp.flags << ", "
                << "adaptive_descriptor_nomask: " << fp.adaptiveDescriptorWithoutMask << ", "
                << "adaptive_descriptor_withmask addr=0x" << fp.adaptiveDescriptorWithMask << ", "
                << "enroll_descriptor addr=0x" << fp.enrollmentDescriptor << ", " << '>';
            return oss.str();
        });


    py::class_<ExtractedFaceprintsElement>(m, "ExtractedFaceprintsElement")
        .def(py::init<>())
        .def("__copy__",  [](const ExtractedFaceprintsElement &self) {
            return ExtractedFaceprintsElement(self);
        })
        .def_readwrite("version", &ExtractedFaceprintsElement::version)
        .def_readwrite("features_type", &ExtractedFaceprintsElement::featuresType)
        .def_readwrite("flags", &ExtractedFaceprintsElement::flags)
        /* featuresVector getter/getter */
        .def_property(
            "features",
            [](ExtractedFaceprintsElement& self) {
                // getter
                return std::vector<feature_t> {std::begin(self.featuresVector), std::end(self.featuresVector)};
            },
            // setter
            [](ExtractedFaceprintsElement& self, const std::vector<feature_t>& new_descriptors) {
                constexpr size_t n_elements = std::extent<decltype(self.featuresVector)>::value;
                static_assert(n_elements == RSID_FEATURES_VECTOR_ALLOC_SIZE,
                              "n_elements!=RSID_FEATURES_VECTOR_ALLOC_SIZE");
                if (new_descriptors.size() != n_elements)
                {
                    throw std::runtime_error("Invalid number of elements in setter. Expected " +
                                             std::to_string(n_elements));
                }
                std::copy(new_descriptors.begin(), new_descriptors.end(), self.featuresVector);
            })


        .def("__repr__", [](const ExtractedFaceprintsElement& fp) {
            std::ostringstream oss;
            oss << "<rsid_py.ExtractedFaceprintsElement "
                << "version=" << fp.version << ", "
                << "feature_type=" << fp.featuresType << ", "
                << "flags=" << fp.flags << ", "
                << "features=" << fp.featuresVector << '>';
            return oss.str();
        });


    py::class_<MatchResultHost>(m, "MatchResult")
        .def(py::init<>())
        .def("__copy__",  [](const MatchResultHost &self) {
            return MatchResultHost(self);
        })
        .def_readwrite("success", &MatchResultHost::success)
        .def_readwrite("should_update", &MatchResultHost::should_update)
        .def_readwrite("score", &MatchResultHost::score)

        .def("__repr__", [](const MatchResultHost& fp) {
            std::ostringstream oss;
            oss << "<rsid_py.MatchResult "
                << "success=" << fp.success << ", "
                << "should_update=" << fp.should_update << ", "
                << "score=" << fp.score << '>';
            return oss.str();
        });
    //
    //

    ExtractedFaceprints pExtractedFaceprints;

    py::class_<FaceAuthenticator>(m, "FaceAuthenticator")
        .def(py::init<>())                          // default ctor
        .def(py::init([](const std::string& port) { // ctor with port to connect
            auto f = std::make_unique<FaceAuthenticator>();
            RSID_THROW_ON_ERROR(f->Connect(SerialConfig {port.c_str()}));
            return f;
        }))

        .def("__enter__", [](FaceAuthenticator& self) { return &self; })
        .def("__exit__", [](FaceAuthenticator& self,
                            py::handle type, py::handle value, py::handle traceback) {
            self.Disconnect();
        })

        .def(
            "connect",
            [](FaceAuthenticator& self, const std::string& port) {
                RSID_THROW_ON_ERROR(self.Connect(SerialConfig {port.c_str()}));
            },
            py::call_guard<py::gil_scoped_release>())

        .def("disconnect", &FaceAuthenticator::Disconnect, py::call_guard<py::gil_scoped_release>())

        .def(
            "cancel", [](FaceAuthenticator& self) { RSID_THROW_ON_ERROR(self.Cancel()); },
            py::call_guard<py::gil_scoped_release>())

        // device mode api
        .def(
            "enroll",
            [](FaceAuthenticator& self, EnrollStatusClbkFun& fn1, EnrollProgressClbkFun& fn2, EnrollHintClbkFun& fn3,
               FaceDetectedClbkFun& fn4, const std::string& user_id) {
                EnrollCallbackPy clbk {fn1, fn2, fn3, fn4};
                RSID_THROW_ON_ERROR(self.Enroll(clbk, user_id.c_str()));
            },
            py::arg("on_result") = EnrollStatusClbkFun {}, py::arg("on_progress") = EnrollProgressClbkFun {},
            py::arg("on_hint") = EnrollHintClbkFun {}, py::arg("on_faces") = FaceDetectedClbkFun {}, py::arg("user_id"),
            py::call_guard<py::gil_scoped_release>())
        .def(
            "enroll_image",
            [](FaceAuthenticator& self, const std::string& user_id, const std::vector<unsigned char>& buffer, int width,
               int height) { return self.EnrollImage(user_id.c_str(), buffer.data(), width, height); },
            py::arg("user_id"), py::arg("buffer"), py::arg("width"), py::arg("height"),
            py::doc("Enroll with image. Buffer should be bgr24"), py::call_guard<py::gil_scoped_release>())
        .def(
            "extract_image_faceprints_for_enroll",
            [&](FaceAuthenticator& self, const std::vector<unsigned char>& buffer, int width, int height) {
                ExtractedFaceprints fp;
                auto status = self.EnrollImageFeatureExtraction("no_used", buffer.data(), width, height, &fp);
                if (status != RealSenseID::EnrollStatus::Success)
                    throw std::runtime_error(RealSenseID::Description(status));
                return fp.data;
            },
            py::arg("buffer"), py::arg("width"), py::arg("height"),
            py::doc("Enroll with image. Buffer should be bgr24"), py::call_guard<py::gil_scoped_release>())

        .def(
            "authenticate",
            [](FaceAuthenticator& self, AuthStatusClbkFun& fn1, AuthHintClbkFun& fn2, FaceDetectedClbkFun& fn3) {
                AuthCallbackPy clbk {fn1, fn2, fn3};
                RSID_THROW_ON_ERROR(self.Authenticate(clbk));
            },
            py::arg("on_result") = AuthStatusClbkFun {}, py::arg("on_hint") = AuthHintClbkFun {},
            py::arg("on_faces") = FaceDetectedClbkFun {}, py::call_guard<py::gil_scoped_release>())

        .def(
            "authenticate_loop",
            [](FaceAuthenticator& self, AuthStatusClbkFun& fn1, AuthHintClbkFun& fn2, FaceDetectedClbkFun& fn3) {
                AuthCallbackPy clbk {fn1, fn2, fn3};
                RSID_THROW_ON_ERROR(self.AuthenticateLoop(clbk));
            },
            py::arg("on_result") = AuthStatusClbkFun {}, py::arg("on_hint") = AuthHintClbkFun {},
            py::arg("on_faces") = FaceDetectedClbkFun {}, py::call_guard<py::gil_scoped_release>())

        // users api
        .def_readonly_static("MAX_USERID_LENGTH", &RealSenseID::MAX_USERID_LENGTH)
        .def(
            "remove_user",
            [](FaceAuthenticator& self, const std::string& user_id) {
                RSID_THROW_ON_ERROR(self.RemoveUser(user_id.c_str()));
            },
            py::arg("user_id"), py::call_guard<py::gil_scoped_release>())

        .def(
            "remove_all_users", [](FaceAuthenticator& self) { RSID_THROW_ON_ERROR(self.RemoveAll()); },
            py::call_guard<py::gil_scoped_release>())

        .def(
            "query_number_of_users",
            [](FaceAuthenticator& self) {
                unsigned int n_users = 0;
                RSID_THROW_ON_ERROR(self.QueryNumberOfUsers(n_users));
                return n_users;
            },
            py::call_guard<py::gil_scoped_release>())

        .def(
            "query_user_ids", [](FaceAuthenticator& self) { return query_users(self); },
            py::call_guard<py::gil_scoped_release>())

        .def(
            "query_device_config",
            [](FaceAuthenticator& self) {
                DeviceConfig device_config;
                RSID_THROW_ON_ERROR(self.QueryDeviceConfig(device_config));
                return device_config;
            },
            py::call_guard<py::gil_scoped_release>())

        .def(
            "set_device_config",
            [](FaceAuthenticator& self, const DeviceConfig device_config) {
                RSID_THROW_ON_ERROR(self.SetDeviceConfig(device_config));
            },
            py::call_guard<py::gil_scoped_release>())

        .def(
            "standby", [](FaceAuthenticator& self) { RSID_THROW_ON_ERROR(self.Standby()); },
            py::call_guard<py::gil_scoped_release>())

        .def(
            "unlock", [](FaceAuthenticator& self) { RSID_THROW_ON_ERROR(self.Unlock()); },
            py::call_guard<py::gil_scoped_release>())


        //
        // host mode api
        //

        .def(
            "extract_faceprints_for_enroll",
            [](FaceAuthenticator& self, FaceprintsEnrollStatusClbkFun& fn1, EnrollProgressClbkFun& fn2,
               EnrollHintClbkFun& fn3, FaceDetectedClbkFun& fn4) {
                FaceprintsEnrollCallbackPy clbk {fn1, fn2, fn3, fn4};
                RSID_THROW_ON_ERROR(self.ExtractFaceprintsForEnroll(clbk));
            },
            py::arg("on_result") = FaceprintsEnrollStatusClbkFun {}, py::arg("on_progress") = EnrollProgressClbkFun {},
            py::arg("on_hint") = EnrollHintClbkFun {}, py::arg("on_faces") = FaceDetectedClbkFun {},
            py::call_guard<py::gil_scoped_release>())


        .def(
            "extract_faceprints_for_auth",
            [](FaceAuthenticator& self, FaceprintsAuthStatusClbkFun& fn1, AuthHintClbkFun& fn2,
               FaceDetectedClbkFun& fn3) {
                FaceprintsAuthCallbackPy clbk {fn1, fn2, fn3};
                RSID_THROW_ON_ERROR(self.ExtractFaceprintsForAuth(clbk));
            },
            py::arg("on_result") = FaceprintsAuthStatusClbkFun {}, py::arg("on_hint") = AuthHintClbkFun {},
            py::arg("on_faces") = FaceDetectedClbkFun {}, py::call_guard<py::gil_scoped_release>())

        .def("match_faceprints",
             [](FaceAuthenticator& self, ExtractedFaceprintsElement& new_faceprints,
                const DBFaceprintsElement& existing_faceprints, DBFaceprintsElement& updated_faceprints,
                const DeviceConfig::MatcherConfidenceLevel& confidence_level) {
                  // wrap with needed classes
                  MatchElement match_element;
                  match_element.data = new_faceprints;

                  Faceprints existing;
                  existing.data = existing_faceprints;

                  Faceprints updated;
                  MatchResultHost match_result;

                  switch (confidence_level)
                  {
                  case DeviceConfig::MatcherConfidenceLevel::High:
                      match_result = self.MatchFaceprints(match_element, existing, updated,
                                                          ThresholdsConfidenceEnum::ThresholdsConfidenceLevel_High);
                      break;
                  case DeviceConfig::MatcherConfidenceLevel::Medium:
                      match_result = self.MatchFaceprints(match_element, existing, updated,
                                                          ThresholdsConfidenceEnum::ThresholdsConfidenceLevel_Medium);
                      break;
                  case DeviceConfig::MatcherConfidenceLevel::Low:
                      match_result = self.MatchFaceprints(match_element, existing, updated,
                                                          ThresholdsConfidenceEnum::ThresholdsConfidenceLevel_Low);
                      break;
                  }

                  updated_faceprints = updated.data;
                  return match_result;
             },
             py::arg("new_faceprints"), py::arg("existing_faceprints"),
             py::arg("updated_faceprints"), py::arg("confidence_level") = DeviceConfig::MatcherConfidenceLevel::High,
             py::call_guard<py::gil_scoped_release>())

        //
        // License api
        //
        .def_static("get_license_key", FaceAuthenticator::GetLicenseKey, py::call_guard<py::gil_scoped_release>())

        .def_static(
            "set_license_key",
            [](const std::string& key) { RSID_THROW_ON_ERROR(FaceAuthenticator::SetLicenseKey(key)); },
            py::call_guard<py::gil_scoped_release>())


        .def(
            "provide_license", [](FaceAuthenticator& self) { RSID_THROW_ON_ERROR(self.ProvideLicense()); },
            py::call_guard<py::gil_scoped_release>())

        .def(
            "enable_license_check_handler",
            [](FaceAuthenticator& self) { self.EnableLicenseCheckHandler(nullptr, nullptr); },
            py::call_guard<py::gil_scoped_release>())

        .def(
            "disable_license_check_handler", [](FaceAuthenticator& self) { self.DisableLicenseCheckHandler(); },
            py::call_guard<py::gil_scoped_release>());
}
