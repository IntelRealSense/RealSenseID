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
#include "RealSenseID/MatcherDefines.h"

#ifdef RSID_SECURE
#include "PacketManager/SecureSession.h"
using Session = RealSenseID::PacketManager::SecureSession;
#else
#include "PacketManager/NonSecureSession.h"
using Session = RealSenseID::PacketManager::NonSecureSession;
#endif // RSID_SECURE

#include <memory>
#include <atomic>
#include <chrono>
#include <string>

#include "IFaceAuthenticator.h"

namespace RealSenseID
{
namespace Impl
{
class FaceAuthenticatorCommon : public IFaceAuthenticator
{
public:
    explicit FaceAuthenticatorCommon(SignatureCallback* callback);
    ~FaceAuthenticatorCommon() override = default;

    FaceAuthenticatorCommon(const FaceAuthenticatorCommon&) = delete;
    FaceAuthenticatorCommon& operator=(const FaceAuthenticatorCommon&) = delete;
    FaceAuthenticatorCommon(FaceAuthenticatorCommon&&) = delete;
    FaceAuthenticatorCommon& operator=(FaceAuthenticatorCommon&&) = delete;

    Status Connect(const SerialConfig& config) override;
    void Disconnect() override;

#ifdef RSID_SECURE
    Status Pair(const char* ecdsaHostPubKey, const char* ecdsaHostPubKeySig, char* ecdsaDevicePubKey) override;
    Status Unpair() override;
#endif

    Status Enroll(EnrollmentCallback& callback, const char* user_id) override;
    EnrollStatus EnrollImage(const char* user_id, const unsigned char* buffer, unsigned int width, unsigned int height) override;
    EnrollStatus EnrollCroppedFaceImage(const char* user_id, const unsigned char* buffer) override;
    EnrollStatus EnrollImageFeatureExtraction(const char* user_id, const unsigned char* buffer, unsigned int width, unsigned int height,
                                              ExtractedFaceprints* faceprints) override;
    Status Authenticate(AuthenticationCallback& callback) override;
    Status AuthenticateLoop(AuthenticationCallback& callback) override;
    Status Cancel() override;
    Status RemoveUser(const char* user_id) override;
    Status RemoveAll() override;

    Status SetDeviceConfig(const DeviceConfig& device_config) override;
    Status QueryDeviceConfig(DeviceConfig& device_config) override;
    Status QueryUserIds(char** user_ids, unsigned int& number_of_users) override;
    Status QueryNumberOfUsers(unsigned int& number_of_users) override;
    Status Standby() override;
    Status Hibernate() override;
    Status Unlock() override;


    Status SendImageToDevice(const unsigned char* buffer, unsigned int width, unsigned int height) override;
    Status ExtractFaceprintsForEnroll(EnrollFaceprintsExtractionCallback& callback) override;
    Status ExtractFaceprintsForAuth(AuthFaceprintsExtractionCallback& callback) override;
    Status ExtractFaceprintsForAuthLoop(AuthFaceprintsExtractionCallback& callback) override;

    MatchResultHost MatchFaceprints(MatchElement& new_faceprints, Faceprints& existing_faceprints, Faceprints& updated_faceprints,
                                    ThresholdsConfidenceEnum matcher_confidence_level) override;

    Status GetUsersFaceprints(Faceprints* user_features, unsigned int& num_of_users) override;
    Status SetUsersFaceprints(UserFaceprints* users_faceprints, unsigned int num_of_users) override;

protected:
#ifdef RSID_SECURE
    // in secure mode sleep less to take into account start session duration
    const std::chrono::milliseconds _loop_interval_no_face {1500};
    const std::chrono::milliseconds _loop_interval_with_face {100};
#else
    const std::chrono::milliseconds _loop_interval_no_face {2100};
    const std::chrono::milliseconds _loop_interval_with_face {600};
#endif
    std::atomic<bool> _cancel_loop {false};
    std::unique_ptr<PacketManager::SerialConnection> _serial;
    Session _session;

    // wait for cancel flag while sleeping upto timeout
    void AuthLoopSleep(std::chrono::milliseconds timeout) const;
    static bool ValidateUserId(const char* user_id);
    Status SendUserFaceprints(UserFaceprints& features);
};
} // namespace Impl
} // namespace RealSenseID
