// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/FaceAuthenticator.h"
#include "RealSenseID/DeviceConfig.h"
#include "Logger.h"
#include "FaceAuthenticatorImpl.h"

namespace RealSenseID
{
#ifdef RSID_SECURE
FaceAuthenticator::FaceAuthenticator(SignatureCallback* callback) : _impl {new FaceAuthenticatorImpl(callback)}
{
}
#else
FaceAuthenticator::FaceAuthenticator() : _impl {new FaceAuthenticatorImpl(nullptr)}
{
}
#endif

FaceAuthenticator::~FaceAuthenticator()
{
    try
    {
        delete _impl;
    }
    catch (...)
    {
    }
}

Status FaceAuthenticator::Connect(const SerialConfig& config)
{
    return _impl->Connect(config);
}

void FaceAuthenticator::Disconnect()
{
    _impl->Disconnect();
}

#ifdef RSID_SECURE
Status FaceAuthenticator::Pair(const char* ecdsa_host_pubKey, const char* ecdsa_host_pubkey_sig, char* ecdsa_device_pubkey)
{
    return _impl->Pair(ecdsa_host_pubKey, ecdsa_host_pubkey_sig, ecdsa_device_pubkey);
}

Status FaceAuthenticator::Unpair()
{
    return _impl->Unpair();
}
#endif // RSID_SECURE

// Perform license check with license server if given status is LicenseCheck and if _enable_license_handler is set to
// true. Return true if license check was performed succesfully or false if it was not required or failed.
template <typename T>
bool FaceAuthenticator::HandleLicenseCheck(T status)
{
    if (T::LicenseCheck != status || !_enable_license_handler)
        return false;

    try
    {
        if (_on_start_license_session != nullptr)
            _on_start_license_session();

        auto license_session_result = _impl->ProvideLicense();

        if (_on_end_license_session != nullptr)
            _on_end_license_session(license_session_result);

        return license_session_result == Status::Ok;
    }
    catch (...)
    {
        return false;
    }
}

#ifdef RSID_NETWORK
// 1. perform the given function and check
// 2. if status is LicenseCheck perform license provision and retry
#define CALL_IMPL(_func, ...)                                                                                                              \
    {                                                                                                                                      \
        auto status = (_impl)->_func(__VA_ARGS__);                                                                                         \
        if (HandleLicenseCheck(status))                                                                                                    \
            return (_impl)->_func(__VA_ARGS__);                                                                                            \
        else                                                                                                                               \
            return status;                                                                                                                 \
    }

#else // if license handler is disabled, just call the function
#define CALL_IMPL(_func, ...)                                                                                                              \
    {                                                                                                                                      \
        return (_impl)->_func(__VA_ARGS__);                                                                                                \
    }
#endif // RSID_NETWORK


Status FaceAuthenticator::Enroll(EnrollmentCallback& callback, const char* user_id)
{
    CALL_IMPL(Enroll, callback, user_id);
    // return _impl->Enroll(callback, user_id);
}

EnrollStatus FaceAuthenticator::EnrollImage(const char* user_id, const unsigned char* buffer, unsigned int width, unsigned int height)
{
    CALL_IMPL(EnrollImage, user_id, buffer, width, height);
    // return _impl->EnrollImage(user_id, buffer, width, height);
}

EnrollStatus FaceAuthenticator::EnrollCroppedFaceImage(const char* user_id, const unsigned char* buffer)
{
    CALL_IMPL(EnrollCroppedFaceImage, user_id, buffer);
    // return _impl->EnrollCroppedFaceImage(user_id, buffer);
}

EnrollStatus FaceAuthenticator::EnrollImageFeatureExtraction(const char* user_id, const unsigned char* buffer, unsigned int width,
                                                             unsigned int height, ExtractedFaceprints* pExtractedFaceprints)
{
    CALL_IMPL(EnrollImageFeatureExtraction, user_id, buffer, width, height, pExtractedFaceprints);
    // return _impl->EnrollImageFeatureExtraction(user_id, buffer, width, height, pExtractedFaceprints);
}

Status FaceAuthenticator::Authenticate(AuthenticationCallback& callback)
{
    CALL_IMPL(Authenticate, callback);
    // return _impl->Authenticate(callback);
}

Status FaceAuthenticator::AuthenticateLoop(AuthenticationCallback& callback)
{
    CALL_IMPL(AuthenticateLoop, callback);
    // return _impl->AuthenticateLoop(callback);
}

Status FaceAuthenticator::Cancel()
{
    return _impl->Cancel();
}

Status FaceAuthenticator::RemoveUser(const char* user_id)
{
    CALL_IMPL(RemoveUser, user_id);
    // return _impl->RemoveUser(user_id);
}

Status FaceAuthenticator::RemoveAll()
{
    CALL_IMPL(RemoveAll);
    // return _impl->RemoveAll();
}

Status FaceAuthenticator::SetDeviceConfig(const DeviceConfig& deviceConfig)
{
    CALL_IMPL(SetDeviceConfig, deviceConfig);
    // return _impl->SetDeviceConfig(deviceConfig);
}

Status FaceAuthenticator::QueryDeviceConfig(DeviceConfig& deviceConfig)
{
    return _impl->QueryDeviceConfig(deviceConfig); // no license check happens for QueryDeviceConfig
}

Status FaceAuthenticator::QueryUserIds(char** user_ids, unsigned int& number_of_users)
{
    CALL_IMPL(QueryUserIds, user_ids, number_of_users);
    // return _impl->QueryUserIds(user_ids, number_of_users);
}

Status FaceAuthenticator::QueryNumberOfUsers(unsigned int& number_of_users)
{
    CALL_IMPL(QueryNumberOfUsers, number_of_users);
    // return _impl->QueryNumberOfUsers(number_of_users);
}

Status FaceAuthenticator::Standby()
{
    return _impl->Standby();
}

Status FaceAuthenticator::Hibernate()
{
    return _impl->Hibernate();
}

Status FaceAuthenticator::Unlock()
{
    return _impl->Unlock();
}


Status FaceAuthenticator::ExtractFaceprintsForEnroll(EnrollFaceprintsExtractionCallback& callback)
{
    CALL_IMPL(ExtractFaceprintsForEnroll, callback);
    // return _impl->ExtractFaceprintsForEnroll(callback);
}

Status FaceAuthenticator::ExtractFaceprintsForAuth(AuthFaceprintsExtractionCallback& callback)
{
    CALL_IMPL(ExtractFaceprintsForAuth, callback);
    // return _impl->ExtractFaceprintsForAuth(callback);
}

Status FaceAuthenticator::ExtractFaceprintsForAuthLoop(AuthFaceprintsExtractionCallback& callback)
{
    CALL_IMPL(ExtractFaceprintsForAuthLoop, callback);
    // return _impl->ExtractFaceprintsForAuthLoop(callback);
}

MatchResultHost FaceAuthenticator::MatchFaceprints(MatchElement& new_faceprints, Faceprints& existing_faceprints,
                                                   Faceprints& updated_faceprints, ThresholdsConfidenceEnum matcher_confidence_level)
{
    return _impl->MatchFaceprints(new_faceprints, existing_faceprints, updated_faceprints, matcher_confidence_level);
}

Status FaceAuthenticator::GetUsersFaceprints(Faceprints* user_features, unsigned int& num_of_users)
{
    CALL_IMPL(GetUsersFaceprints, user_features, num_of_users);
    // return _impl->GetUsersFaceprints(user_features, num_of_users);
}

Status FaceAuthenticator::SetUsersFaceprints(UserFaceprints_t* user_features, unsigned int num_of_users)
{
    CALL_IMPL(SetUsersFaceprints, user_features, num_of_users);
    // return _impl->SetUsersFaceprints(user_features, num_of_users);
}


Status FaceAuthenticator::SetLicenseKey(const std::string& license_key)
{
    return FaceAuthenticatorImpl::SetLicenseKey(license_key);
}


std::string FaceAuthenticator::GetLicenseKey()
{
    return FaceAuthenticatorImpl::GetLicenseKey();
}

Status FaceAuthenticator::ProvideLicense()
{
    return _impl->ProvideLicense();
}

void FaceAuthenticator::EnableLicenseCheckHandler(OnStartLicenseSession on_start_session, OnEndLicenseSession on_end_session)
{
    _enable_license_handler = true;
    _on_start_license_session = on_start_session;
    _on_end_license_session = on_end_session;
}

void FaceAuthenticator::DisableLicenseCheckHandler()
{
    _enable_license_handler = false;
    _on_start_license_session = nullptr;
    _on_end_license_session = nullptr;
}

} // namespace RealSenseID
