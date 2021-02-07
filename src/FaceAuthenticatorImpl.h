// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/SerialConfig.h"
#include "RealSenseID/EnrollmentCallback.h"
#include "RealSenseID/AuthenticationCallback.h"
#include "RealSenseID/Status.h"
#include "RealSenseID/SignatureCallback.h"
#include "RealSenseID/Faceprints.h"
#include "RealSenseID/MatchResult.h"
#include "PacketManager/SecureHostSession.h"

#include <memory>

namespace RealSenseID
{
struct AuthConfig;
class AuthFaceprintsExtractionCallback;

class FaceAuthenticatorImpl
{
public:
    explicit FaceAuthenticatorImpl(SignatureCallback* callback);

    ~FaceAuthenticatorImpl() = default;

    FaceAuthenticatorImpl(const FaceAuthenticatorImpl&) = delete;
    FaceAuthenticatorImpl& operator=(const FaceAuthenticatorImpl&) = delete;

    Status Connect(const SerialConfig& config);
#ifdef ANDROID
    Status Connect(int fileDescriptor, int readEndpointAddress, int writeEndpointAddress);
#endif

    void Disconnect();

    Status Pair(const char* ecdsaHostPubKey, const char* ecdsaHostPubKeySig, char* ecdsaDevicePubKey);
    Status SetAuthSettings(const AuthConfig& auth_config);
    Status QueryAuthSettings(AuthConfig& auth_config);
    Status QueryUserIds(char** user_ids, unsigned int& number_of_users);
    Status QueryNumberOfUsers(unsigned int& number_of_users);
    Status Standby();

    Status Enroll(EnrollmentCallback& callback, const char* user_id);
    Status EnrollExtractFaceprints(EnrollmentCallback& callback, const char* user_id, Faceprints& faceprints);
    Status Authenticate(AuthenticationCallback& callback);
    Status AuthenticateExtractFaceprints(AuthFaceprintsExtractionCallback& callback, Faceprints& faceprints);
    Status AuthenticateLoop(AuthenticationCallback& callback);
    Status AuthenticateExtractFaceprintsLoop(AuthFaceprintsExtractionCallback& callback, Faceprints& faceprints);    

    Status Cancel();
    Status RemoveUser(const char* user_id);
    Status RemoveAllUsers();

    MatchResult MatchFaceprints(Faceprints& new_faceprints, Faceprints& existing_faceprints,
                                Faceprints& updated_faceprints);

private:
    PacketManager::SecureHostSession _session;
    const std::chrono::milliseconds _enroll_session_timeout {10000};
    const std::chrono::milliseconds _auth_session_timeout {5000};
    std::unique_ptr<PacketManager::SerialConnection> _serial;

    static bool ValidateUserId(const char* user_id);
};
} // namespace RealSenseID
