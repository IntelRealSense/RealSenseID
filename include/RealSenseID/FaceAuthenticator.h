// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/AuthConfig.h"
#include "RealSenseID/AuthenticationCallback.h"
#include "RealSenseID/AuthFaceprintsExtractionCallback.h"
#include "RealSenseID/EnrollFaceprintsExtractionCallback.h"
#include "RealSenseID/EnrollmentCallback.h"
#include "RealSenseID/Faceprints.h"
#include "RealSenseID/MatchResult.h"
#include "RealSenseID/SerialConfig.h"
#include "RealSenseID/SignatureCallback.h"
#include "RealSenseID/Status.h"
#ifdef ANDROID
#include "RealSenseID/AndroidSerialConfig.h"
#endif

namespace RealSenseID
{
class FaceAuthenticatorImpl;

/**
 * Face authenticator class.
 * Provides face authentication operations using the device.
 */
class RSID_API FaceAuthenticator
{
public:
#ifdef RSID_SECURE
    explicit FaceAuthenticator(SignatureCallback* callback);
#else // RSID_SECUURE
    FaceAuthenticator();
#endif


    ~FaceAuthenticator();

    FaceAuthenticator(const FaceAuthenticator&) = delete;
    FaceAuthenticator& operator=(const FaceAuthenticator&) = delete;

    /**
     * Connect to device using the given serial config
     * Reconnect if already connected.
     *
     * @param[in] config Serial configuration
     * @return connection status
     */
    Status Connect(const SerialConfig& config);

#ifdef ANDROID
    /**
     * Connect to device using the given Android serial config
     * reconnect if already connected.
     * 
     * @param[in] Android config Serial configuration
     * @return connection status
     */
    Status Connect(const AndroidSerialConfig& config);
#endif

    /**
     * Disconnect from the device.
     */
    void Disconnect();

#ifdef RSID_SECURE
    /**
     * Send updated host ecdsa key to device, sign it with previous ecdsa key (at first pair can sign with dummy key)
     * If the operation successfully, the output is device's ecdsa key which stored in the ecdsa_device_pubkey buffer.
     *
     * @param ecdsa_host_pubKey 64 bytes of host's public key
     * @param ecdsa_host_pubkey_sig 32 bytes of host's public key signature
     * @param ecdsa_device_pubkey buffer of 64 bytes to store of device's public key response
     * @return Status::Ok on successfull pairing operation.
     */
    Status Pair(const char* ecdsa_host_pubKey, const char* ecdsa_host_pubkey_sig, char* ecdsa_device_pubkey);

    /**
     * Unpair host with connected device.
     * This will disable security.
     *
     * @return Status::Ok on successfull pairing operation.
     */
    Status Unpair();
#endif // RSID_SECURE

    /**
     * Enroll a user.
     *
     * Starts the enrollment process, which starts the camera, captures frames and then extracts
     * facial information at different poses.
     * During the process callbacks will be called to provide information if needed.
     * Once process is done, camera will be closed properly and device will be in ready state.
     *
     * @param[in] callback User defined callback to handle the process updates.
     * @param[in] user_id Null terminated C string of ascii chars. Max user id size is 15 bytes (max total of 16 bytes
     * including the terminating zero byte). user.
     * @return Status (Status::Ok on success).
     */
    Status Enroll(EnrollmentCallback& callback, const char* user_id);

    /**
     * Attempt to authenticate.
     *
     * Starts the authentication procedure, which starts the camera, captures frames and tries to match
     * the user in front of the camera to the enrolled users.
     * During the process callbacks will be called to provide information if needed.
     * Once process is done, camera will be closed properly and device will be in ready state.
     *
     * @param[in] callback User defined callback object to handle the process updates.
     * @return Status (Status::Ok on success).
     */
    Status Authenticate(AuthenticationCallback& callback);

    /**
     * Start Authentication Loop.
     *
     * Starts infinite authentication loop. Call Cancel to stop it.
     * @param[in] callback User defined callback object to handle the process updates.
     * @return Status (Status::Ok on success).
     */
    Status AuthenticateLoop(AuthenticationCallback& callback);

    /**
     * Cancel currently running operation.
     *
     * @return Status (Status::Ok on success).
     */
    Status Cancel();

    /**
     * Attempt to remove specific user from the device.
     *
     * @param[in] user_id Unique id (null terminated ascii, 16 bytes max) of the user that should be removed.
     * @return Status::Ok on success.
     */
    Status RemoveUser(const char* user_id);

    /**
     * Attempt to remove all users from the device.
     *
     * @return Status::Ok on success.
     */
    Status RemoveAll();

    /**
     * Apply advanced settings to FW.
     *
     * @param[in] authConfig config with settings.
     * @return Status::Ok on success.
     */
    Status SetAuthSettings(const AuthConfig& authConfig);

    /**
     * Query advanced settings to FW.
     * @param[out] authentication config with settings.
     * @return Status::Ok on success.
     */
    Status QueryAuthSettings(AuthConfig& auth_config);

    /**
     * Query the device about all enrolled users.
     * @param[out] pre-allocated array of user ids.
     * @param[in/out] number of users to retrieve.
     * @return Status::Ok on success.
     */
    Status QueryUserIds(char** user_ids, unsigned int& number_of_users_in_out);

    /**
     * Query the device about the number of enrolled users.
     * @param[out] number of users.
     * @return Status::Ok on success.
     */
    Status QueryNumberOfUsers(unsigned int& number_of_users);

    /**
     * Prepare device to standby - for now it's saving database of users to flash.
     * @return Status::Ok on success.
     */
    Status Standby();

    /*************************************************/
    /************** Server Mode Methods **************/
    /*************************************************/
    /**
     * Attempt to extract faceprints using enrollment flow.
     *
     * Starts the enrollment process, which starts the camera, captures frames, extracts
     * faceprints of different face-poses, and sends them to the host.
     * During the process callbacks will be called to provide information if needed.
     * Once process is done, camera will be closed properly and device will be in ready state.
     *
     * @param[in] callback User defined callback to handle the process updates.
     * @param[in] user_id Unique id (null terminated ascii, 16 bytes max) that will be given to the current enrolled
     * user.
     * @param[in] faceprints an object which will contain the faceprints extracted from the device.
     * @return EnrollStatus enroll result for this operation.
     */
    Status ExtractFaceprintsForEnroll(EnrollFaceprintsExtractionCallback& callback);

    /**
     * Attempt to extract faceprints using authentication flow.
     *
     * Starts the authentication procedure, which starts the camera, captures frames, extracts faceprints,
     * and sends them to the host.
     * During the process callbacks will be called to provide information if needed.
     * Once process is done, camera will be closed properly and device will be in ready state.
     *
     * @param[in] callback User defined callback object to handle the process updates.
     * @param[in] faceprints an object which will contain the faceprints extracted from the device.
     * @return AuthenticateStatus faceprints extraction result for this operation.
     */
    Status ExtractFaceprintsForAuth(AuthFaceprintsExtractionCallback& callback);

    /**
     * Attempt faceprints extraction in a loop using authentication flow.
     *
     * Starts infinite authentication loop. Call Cancel to stop it.
     * @param[in] callback User defined callback object to handle the process updates.
     * @param[in] faceprints an object which will contain the faceprints extracted from the device.
     * @return Status (Status::Ok on success).
     */
    Status ExtractFaceprintsForAuthLoop(AuthFaceprintsExtractionCallback& callback);

    /**
     * Match two faceprints to each other.
     *
     * Calculates a score for how similar the two faceprints are, and returns a prediction for whether the
     * two faceprints belong to the same person.
     *
     * @param[in] new_faceprints faceprints which were extracted from a single image of a person.
     * @param[in] existing_faceprints faceprints which were calculated from one or more images of the same person.
     * @param[in] updated_faceprints a placeholder to write the updated-faceprints into, if the match was successful.
     * @return MatchResult match result, the 'success' field indicates if the two faceprints belong to the same person.
     */
    MatchResult MatchFaceprints(Faceprints& new_faceprints, Faceprints& existing_faceprints,
                                Faceprints& updated_faceprints);

private:
    FaceAuthenticatorImpl* _impl = nullptr;
};
} // namespace RealSenseID
