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

SerialStatus FaceAuthenticator::Connect(const SerialConfig& config)
{
    return _impl->Connect(config);
}

SerialStatus FaceAuthenticator::SetAuthSettings(const RealSenseID::AuthConfig& authConfig)
{
    return _impl->SetAuthSettings(authConfig);
}

void FaceAuthenticator::Disconnect()
{
    _impl->Disconnect();
}


EnrollStatus FaceAuthenticator::Enroll(EnrollmentCallback& callback, const char* user_id)
{
    return _impl->Enroll(callback, user_id);
}

AuthenticateStatus FaceAuthenticator::Authenticate(AuthenticationCallback& callback)
{
    return _impl->Authenticate(callback);
}

AuthenticateStatus FaceAuthenticator::AuthenticateLoop(AuthenticationCallback& callback)
{
    return _impl->AuthenticateLoop(callback);
}

SerialStatus FaceAuthenticator::Cancel()
{
    return _impl->Cancel();
}

SerialStatus FaceAuthenticator::RemoveUser(const char* user_id)
{
    return _impl->RemoveUser(user_id);
}

SerialStatus FaceAuthenticator::RemoveAll()
{
    return _impl->RemoveAllUsers();
}

} // namespace RealSenseID
