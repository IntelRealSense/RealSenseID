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

#ifdef ANDROID
Status FaceAuthenticator::Connect(const AndroidSerialConfig& config)
{
    return _impl->Connect(config);
}
#endif

void FaceAuthenticator::Disconnect()
{
    _impl->Disconnect();
}

#ifdef RSID_SECURE
Status FaceAuthenticator::Pair(const char* ecdsa_host_pubKey, const char* ecdsa_host_pubkey_sig,
                               char* ecdsa_device_pubkey)
{
    return _impl->Pair(ecdsa_host_pubKey, ecdsa_host_pubkey_sig, ecdsa_device_pubkey);
}

Status FaceAuthenticator::Unpair()
{
    return _impl->Unpair();
}
#endif // RSID_SECURE

Status FaceAuthenticator::Enroll(EnrollmentCallback& callback, const char* user_id)
{
    return _impl->Enroll(callback, user_id);
}

Status FaceAuthenticator::Authenticate(AuthenticationCallback& callback)
{
    return _impl->Authenticate(callback);
}

Status FaceAuthenticator::AuthenticateLoop(AuthenticationCallback& callback)
{
    return _impl->AuthenticateLoop(callback);
}

Status FaceAuthenticator::Cancel()
{
    return _impl->Cancel();
}

Status FaceAuthenticator::RemoveUser(const char* user_id)
{
    return _impl->RemoveUser(user_id);
}

Status FaceAuthenticator::RemoveAll()
{
    return _impl->RemoveAll();
}

Status FaceAuthenticator::SetDeviceConfig(const DeviceConfig& deviceConfig)
{
    return _impl->SetDeviceConfig(deviceConfig);
}

Status FaceAuthenticator::QueryDeviceConfig(DeviceConfig& deviceConfig)
{
    return _impl->QueryDeviceConfig(deviceConfig);
}

Status FaceAuthenticator::QueryUserIds(char** user_ids, unsigned int& number_of_users)
{
    return _impl->QueryUserIds(user_ids, number_of_users);
}

Status FaceAuthenticator::QueryNumberOfUsers(unsigned int& number_of_users)
{
    return _impl->QueryNumberOfUsers(number_of_users);
}

Status FaceAuthenticator::Standby()
{
    return _impl->Standby();
}

Status FaceAuthenticator::ExtractFaceprintsForEnroll(EnrollFaceprintsExtractionCallback& callback)
{
    return _impl->ExtractFaceprintsForEnroll(callback);
}

Status FaceAuthenticator::ExtractFaceprintsForAuth(AuthFaceprintsExtractionCallback& callback)
{
    return _impl->ExtractFaceprintsForAuth(callback);
}

Status FaceAuthenticator::ExtractFaceprintsForAuthLoop(AuthFaceprintsExtractionCallback& callback)
{
    return _impl->ExtractFaceprintsForAuthLoop(callback);
}

MatchResultHost FaceAuthenticator::MatchFaceprints(Faceprints& new_faceprints, Faceprints& existing_faceprints, Faceprints& updated_faceprints)
{
    return _impl->MatchFaceprints(new_faceprints, existing_faceprints, updated_faceprints);
}

Status FaceAuthenticator::GetUsersFaceprints(Faceprints* user_features, unsigned int&num_of_users)
{
    return _impl->GetUsersFaceprints(user_features, num_of_users);
}

Status FaceAuthenticator::SetUsersFaceprints (UserFaceprints * user_features, unsigned int num_of_users)
{
    return _impl->SetUsersFaceprints(user_features, num_of_users);
}

} // namespace RealSenseID
