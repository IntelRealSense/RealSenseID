// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/SerialConfig.h"
#include "RealSenseID/EnrollmentCallback.h"
#include "RealSenseID/EnrollStatus.h"
#include "RealSenseID/AuthenticationCallback.h"
#include "RealSenseID/AuthenticateStatus.h"
#include "RealSenseID/SerialStatus.h"
#include "RealSenseID/SignatureCallback.h"
#include "PacketManager/SecureHostSession.h"

#include <memory>

namespace RealSenseID
{
struct AuthConfig;

class FaceAuthenticatorImpl
{
public:
    explicit FaceAuthenticatorImpl(SignatureCallback* callback);

    ~FaceAuthenticatorImpl() = default;

    FaceAuthenticatorImpl(const FaceAuthenticatorImpl&) = delete;
    FaceAuthenticatorImpl& operator=(const FaceAuthenticatorImpl&) = delete;

    SerialStatus Connect(const SerialConfig& config);
    void Disconnect();
    SerialStatus SetAuthSettings(const RealSenseID::AuthConfig& auth_config);

    EnrollStatus Enroll(EnrollmentCallback& callback, const char* user_id);
    AuthenticateStatus Authenticate(AuthenticationCallback& callback);
    AuthenticateStatus AuthenticateLoop(AuthenticationCallback& callback);
    SerialStatus Cancel();
    SerialStatus RemoveUser(const char* user_id);
    SerialStatus RemoveAllUsers();

private:
    using session_t = PacketManager::SecureHostSession;
    session_t _session;
    const std::chrono::milliseconds _enroll_session_timeout {10000};
    const std::chrono::milliseconds _auth_session_timeout {5000};

    std::unique_ptr<PacketManager::SerialConnection> _serial;
};
} // namespace RealSenseID
