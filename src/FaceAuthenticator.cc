// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/FaceAuthenticator.h"
#include "RealSenseID/AuthConfig.h"
#include "Logger.h"
#include "FaceAuthenticatorImpl.h"

namespace RealSenseID
{
FaceAuthenticator::FaceAuthenticator(SignatureCallback* callback) : _impl {new FaceAuthenticatorImpl(callback)}
{
}

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
Status FaceAuthenticator::Connect(int fileDescriptor, int readEndpointAddress, int writeEndpointAddress)
{
    return _impl->Connect(fileDescriptor, readEndpointAddress, writeEndpointAddress);
}
#endif

Status FaceAuthenticator::Pair(const char* ecdsa_host_pubKey, const char* ecdsa_host_pubkey_sig, char* ecdsa_device_pubkey)
{
    return _impl->Pair(ecdsa_host_pubKey, ecdsa_host_pubkey_sig, ecdsa_device_pubkey);
}

Status FaceAuthenticator::SetAuthSettings(const AuthConfig& authConfig)
{
    return _impl->SetAuthSettings(authConfig);
}

Status FaceAuthenticator::QueryAuthSettings(AuthConfig& authConfig)
{
    return _impl->QueryAuthSettings(authConfig);
}
Status FaceAuthenticator::QueryNumberOfUsers(unsigned int & number_of_users)
{
    return _impl->QueryNumberOfUsers(number_of_users);
}
Status FaceAuthenticator::QueryUserIds(char** user_ids, unsigned int& number_of_users)
{
    return _impl->QueryUserIds(user_ids, number_of_users);
}
void FaceAuthenticator::Disconnect()
{
    _impl->Disconnect();
}

Status FaceAuthenticator::Standby()
{
    return _impl->Standby();
}

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
    return _impl->RemoveAllUsers();
}

Status FaceAuthenticator::AuthenticateExtractFaceprints(AuthFaceprintsExtractionCallback& callback,
                                                                  Faceprints& faceprints)
{
    return _impl->AuthenticateExtractFaceprints(callback, faceprints);
}

Status FaceAuthenticator::AuthenticateExtractFaceprintsLoop(AuthFaceprintsExtractionCallback& callback, Faceprints& faceprints)
{
    return _impl->AuthenticateExtractFaceprintsLoop(callback, faceprints);
}

Status FaceAuthenticator::EnrollExtractFaceprints(EnrollmentCallback& callback,
                                                      const char* user_id, Faceprints& faceprints)
{
    return _impl->EnrollExtractFaceprints(callback, user_id, faceprints);
}

MatchResult FaceAuthenticator::MatchFaceprints(Faceprints& new_faceprints, Faceprints& existing_faceprints,
                                               Faceprints& updated_faceprints)
{
    return _impl->MatchFaceprints(new_faceprints, existing_faceprints, updated_faceprints);
}

} // namespace RealSenseID
