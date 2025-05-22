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
#include "RealSenseID/Status.h"
#include "RealSenseID/MatcherDefines.h"

#ifdef RSID_SECURE
#include "PacketManager/SecureSession.h"
using Session = RealSenseID::PacketManager::SecureSession;
#else
#include "PacketManager/NonSecureSession.h"
using Session = RealSenseID::PacketManager::NonSecureSession;
#endif // RSID_SECURE

namespace RealSenseID
{
namespace Impl
{
class IFaceAuthenticator
{
public:
    virtual ~IFaceAuthenticator() = default;
    virtual Status Connect(const SerialConfig& config) = 0;
    virtual void Disconnect() = 0;

#ifdef RSID_SECURE
    virtual Status Pair(const char* ecdsaHostPubKey, const char* ecdsaHostPubKeySig, char* ecdsaDevicePubKey) = 0;
    virtual Status Unpair() = 0;
#endif

    virtual Status Enroll(EnrollmentCallback& callback, const char* user_id) = 0;
    virtual EnrollStatus EnrollImage(const char* user_id, const unsigned char* buffer, unsigned int width, unsigned int height) = 0;
    virtual EnrollStatus EnrollCroppedFaceImage(const char* user_id, const unsigned char* buffer) = 0;
    virtual EnrollStatus EnrollImageFeatureExtraction(const char* user_id, const unsigned char* buffer, unsigned int width,
                                                      unsigned int height, ExtractedFaceprints* faceprints) = 0;
    virtual Status Authenticate(AuthenticationCallback& callback) = 0;
    virtual Status AuthenticateLoop(AuthenticationCallback& callback) = 0;
    virtual Status Cancel() = 0;
    virtual Status RemoveUser(const char* user_id) = 0;
    virtual Status RemoveAll() = 0;

    virtual Status SetDeviceConfig(const DeviceConfig& device_config) = 0;
    virtual Status QueryDeviceConfig(DeviceConfig& device_config) = 0;
    virtual Status QueryUserIds(char** user_ids, unsigned int& number_of_users) = 0;
    virtual Status QueryNumberOfUsers(unsigned int& number_of_users) = 0;
    virtual Status Standby() = 0;
    virtual Status Hibernate() = 0;
    virtual Status Unlock() = 0;


    virtual Status SendImageToDevice(const unsigned char* buffer, unsigned int width, unsigned int height) = 0;
    virtual Status ExtractFaceprintsForEnroll(EnrollFaceprintsExtractionCallback& callback) = 0;
    virtual Status ExtractFaceprintsForAuth(AuthFaceprintsExtractionCallback& callback) = 0;
    virtual Status ExtractFaceprintsForAuthLoop(AuthFaceprintsExtractionCallback& callback) = 0;

    virtual MatchResultHost MatchFaceprints(MatchElement& new_faceprints, Faceprints& existing_faceprints, Faceprints& updated_faceprints,
                                            ThresholdsConfidenceEnum matcher_confidence_level) = 0;

    virtual Status GetUsersFaceprints(Faceprints* user_features, unsigned int& num_of_users) = 0;
    virtual Status SetUsersFaceprints(UserFaceprints* users_faceprints, unsigned int num_of_users) = 0;
};

} // namespace Impl
} // namespace RealSenseID
