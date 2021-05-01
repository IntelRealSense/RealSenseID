// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/DeviceConfig.h"
#include "RealSenseID/AuthenticationCallback.h"
#include "RealSenseID/AuthFaceprintsExtractionCallback.h"
#include "RealSenseID/EnrollFaceprintsExtractionCallback.h"
#include "RealSenseID/EnrollmentCallback.h"
#include "RealSenseID/Faceprints.h"
#include "RealSenseID/SerialConfig.h"
#include "RealSenseID/SignatureCallback.h"
#include "RealSenseID/Status.h"
#include "RealSenseID/MatchResultHost.h"

#ifdef ANDROID
#include "RealSenseID/AndroidSerialConfig.h"
#endif
#ifdef RSID_SECURE
#include "PacketManager/SecureSession.h"
using Session = RealSenseID::PacketManager::SecureSession;
#else
#include "PacketManager/NonSecureSession.h"
using Session = RealSenseID::PacketManager::NonSecureSession;
#endif // RSID_SECURE

#include <memory>

namespace RealSenseID
{
struct SecureVersionDescriptor;

class FaceAuthenticatorImpl
{
public:
    explicit FaceAuthenticatorImpl(SignatureCallback* callback);

    ~FaceAuthenticatorImpl() = default;

    FaceAuthenticatorImpl(const FaceAuthenticatorImpl&) = delete;
    FaceAuthenticatorImpl& operator=(const FaceAuthenticatorImpl&) = delete;

    Status Connect(const SerialConfig& config);
#ifdef ANDROID
    Status Connect(const AndroidSerialConfig& config);
#endif

    void Disconnect();
#ifdef RSID_SECURE
    Status Pair(const char* ecdsaHostPubKey, const char* ecdsaHostPubKeySig, char* ecdsaDevicePubKey);
    Status Unpair();
#endif // RSID_SECURE

    Status Enroll(EnrollmentCallback& callback, const char* user_id);
    Status Authenticate(AuthenticationCallback& callback);
    Status AuthenticateLoop(AuthenticationCallback& callback);
    Status DetectSpoof(AuthenticationCallback& callback);
    Status Cancel();
    Status RemoveUser(const char* user_id);
    Status RemoveAll();

    Status SetDeviceConfig(const DeviceConfig& device_config);
    Status QueryDeviceConfig(DeviceConfig& device_config);
    Status QueryUserIds(char** user_ids, unsigned int& number_of_users);
    Status QueryNumberOfUsers(unsigned int& number_of_users);
    Status Standby();

    Status ExtractFaceprintsForEnroll(EnrollFaceprintsExtractionCallback& callback);
    Status ExtractFaceprintsForAuth(AuthFaceprintsExtractionCallback& callback);
    Status ExtractFaceprintsForAuthLoop(AuthFaceprintsExtractionCallback& callback);
    MatchResultHost MatchFaceprints(Faceprints& new_faceprints, Faceprints& existing_faceprints, Faceprints& updated_faceprints);

    Status GetUserFeatures(const char* user_id, Faceprints& user_faceprints);
    Status SetUserFeatures(const char* user_id, Faceprints& user_faceprints);

private:
    std::unique_ptr<PacketManager::SerialConnection> _serial;
    Session _session;

    static bool ValidateUserId(const char* user_id);
};
} // namespace RealSenseID
