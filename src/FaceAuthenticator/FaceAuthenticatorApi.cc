// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/FaceAuthenticator.h"
#include "RealSenseID/DeviceConfig.h"
#include "RealSenseID/Version.h"
#include "RealSenseID/DiscoverDevices.h"
#include "Impl/FaceAuthenticatorCommon.h"
#include "Impl/FaceAuthenticatorF45x.h"
#include "Impl/FaceAuthenticatorF46x.h"
#include "Logger.h"

#include <stdexcept>

static const char* LOG_TAG = "FaceAuthenticatorApi";

namespace RealSenseID
{
#ifdef RSID_SECURE
FaceAuthenticator::FaceAuthenticator(SignatureCallback* callback, DeviceType device_type)
{
    switch (device_type)
    {
    case DeviceType::F45x:
        _impl = new Impl::FaceAuthenticatorF45x(callback);
        break;
    case DeviceType::F46x:
        throw std::invalid_argument("RSID_SECURE is not supported in F46x");
    default:
        throw std::invalid_argument("Unknown device type");
    }
}
#else
FaceAuthenticator::FaceAuthenticator(DeviceType device_type)
{
    switch (device_type)
    {
    case DeviceType::F45x:
        _impl = new Impl::FaceAuthenticatorF45x();
        break;
    case DeviceType::F46x:
        _impl = new Impl::FaceAuthenticatorF46x();
        break;
    default:
        LOG_ERROR(LOG_TAG, "Unknown device type");
        throw std::invalid_argument("Unknown device type");
    }
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
    _impl = nullptr;
}

// Move constructor
FaceAuthenticator::FaceAuthenticator(FaceAuthenticator&& other) noexcept
{
    _impl = other._impl;
    other._impl = nullptr;
}

// Move assignment
FaceAuthenticator& FaceAuthenticator::operator=(FaceAuthenticator&& other) noexcept
{
    if (this != &other)
    {
        delete _impl;
        _impl = other._impl;
        other._impl = nullptr;
    }
    return *this;
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


#define WITH_LICENSE_CHECK(_func, ...) return (_impl)->_func(__VA_ARGS__);

Status FaceAuthenticator::Enroll(EnrollmentCallback& callback, const char* user_id)
{
    WITH_LICENSE_CHECK(Enroll, callback, user_id);
    // return _impl->Enroll(callback, user_id);
}

EnrollStatus FaceAuthenticator::EnrollImage(const char* user_id, const unsigned char* buffer, unsigned int width, unsigned int height)
{
    WITH_LICENSE_CHECK(EnrollImage, user_id, buffer, width, height);
    // return _impl->EnrollImage(user_id, buffer, width, height);
}

EnrollStatus FaceAuthenticator::EnrollCroppedFaceImage(const char* user_id, const unsigned char* buffer)
{
    WITH_LICENSE_CHECK(EnrollCroppedFaceImage, user_id, buffer);
    // return _impl->EnrollCroppedFaceImage(user_id, buffer);
}

EnrollStatus FaceAuthenticator::EnrollImageFeatureExtraction(const char* user_id, const unsigned char* buffer, unsigned int width,
                                                             unsigned int height, ExtractedFaceprints* pExtractedFaceprints)
{
    WITH_LICENSE_CHECK(EnrollImageFeatureExtraction, user_id, buffer, width, height, pExtractedFaceprints);
    // return _impl->EnrollImageFeatureExtraction(user_id, buffer, width, height, pExtractedFaceprints);
}

Status FaceAuthenticator::Authenticate(AuthenticationCallback& callback)
{
    WITH_LICENSE_CHECK(Authenticate, callback);
    // return _impl->Authenticate(callback);
}

Status FaceAuthenticator::AuthenticateLoop(AuthenticationCallback& callback)
{
    WITH_LICENSE_CHECK(AuthenticateLoop, callback);
    // return _impl->AuthenticateLoop(callback);
}

Status FaceAuthenticator::Cancel()
{
    return _impl->Cancel();
}

Status FaceAuthenticator::RemoveUser(const char* user_id)
{
    WITH_LICENSE_CHECK(RemoveUser, user_id);
    // return _impl->RemoveUser(user_id);
}

Status FaceAuthenticator::RemoveAll()
{
    WITH_LICENSE_CHECK(RemoveAll);
    // return _impl->RemoveAll();
}

Status FaceAuthenticator::SetDeviceConfig(const DeviceConfig& deviceConfig)
{
    WITH_LICENSE_CHECK(SetDeviceConfig, deviceConfig);
    // return _impl->SetDeviceConfig(deviceConfig);
}

Status FaceAuthenticator::QueryDeviceConfig(DeviceConfig& deviceConfig)
{
    return _impl->QueryDeviceConfig(deviceConfig);
}

Status FaceAuthenticator::QueryUserIds(char** user_ids, unsigned int& number_of_users)
{
    WITH_LICENSE_CHECK(QueryUserIds, user_ids, number_of_users);
    // return _impl->QueryUserIds(user_ids, number_of_users);
}

Status FaceAuthenticator::QueryNumberOfUsers(unsigned int& number_of_users)
{
    WITH_LICENSE_CHECK(QueryNumberOfUsers, number_of_users);
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
    WITH_LICENSE_CHECK(ExtractFaceprintsForEnroll, callback);
    // return _impl->ExtractFaceprintsForEnroll(callback);
}

Status FaceAuthenticator::ExtractFaceprintsForAuth(AuthFaceprintsExtractionCallback& callback)
{
    WITH_LICENSE_CHECK(ExtractFaceprintsForAuth, callback);
    // return _impl->ExtractFaceprintsForAuth(callback);
}

Status FaceAuthenticator::ExtractFaceprintsForAuthLoop(AuthFaceprintsExtractionCallback& callback)
{
    WITH_LICENSE_CHECK(ExtractFaceprintsForAuthLoop, callback);
    // return _impl->ExtractFaceprintsForAuthLoop(callback);
}

MatchResultHost FaceAuthenticator::MatchFaceprints(MatchElement& new_faceprints, Faceprints& existing_faceprints,
                                                   Faceprints& updated_faceprints, ThresholdsConfidenceEnum matcher_confidence_level)
{
    return _impl->MatchFaceprints(new_faceprints, existing_faceprints, updated_faceprints, matcher_confidence_level);
}

Status FaceAuthenticator::GetUsersFaceprints(Faceprints* user_features, unsigned int& num_of_users)
{
    WITH_LICENSE_CHECK(GetUsersFaceprints, user_features, num_of_users);
    // return _impl->GetUsersFaceprints(user_features, num_of_users);
}

Status FaceAuthenticator::SetUsersFaceprints(UserFaceprints* user_features, unsigned int num_of_users)
{
    WITH_LICENSE_CHECK(SetUsersFaceprints, user_features, num_of_users);
    // return _impl->SetUsersFaceprints(user_features, num_of_users);
}
} // namespace RealSenseID
