// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/Version.h"
#include "RealSenseID/DeviceConfig.h"
#include "RealSenseID/AuthenticationCallback.h"
#include "RealSenseID/AuthFaceprintsExtractionCallback.h"
#include "RealSenseID/EnrollFaceprintsExtractionCallback.h"
#include "RealSenseID/EnrollmentCallback.h"
#include "RealSenseID/Faceprints.h"
#include "RealSenseID/SerialConfig.h"

#include "RealSenseID/Status.h"
#include "RealSenseID/MatcherDefines.h"
#include <cstddef>
#include <string>

#ifdef RSID_SECURE
#include "RealSenseID/SignatureCallback.h"
#endif

namespace RealSenseID
{
/**
 *  Max user id size is 30 bytes, 31 bytes including '\0'
 */
static constexpr size_t MAX_USERID_LENGTH = RSID_MAX_USER_ID_LENGTH_IN_DB;

// Forward declaration of the implementation class
namespace Impl
{
class IFaceAuthenticator;
}

/**
 * Face authenticator class.
 * Provides face authentication operations using the device.
 *
 * @note Supports move semantics. Moved-from object should not be used
 */
class RSID_API FaceAuthenticator
{
public:
#ifdef RSID_SECURE
    explicit FaceAuthenticator(SignatureCallback* callback, DeviceType deviceType = DeviceType::F45x);
#else // RSID_SECURE
    explicit FaceAuthenticator(DeviceType device_type = DeviceType::F45x);
#endif

    ~FaceAuthenticator();
    FaceAuthenticator(const FaceAuthenticator&) = delete;
    FaceAuthenticator& operator=(const FaceAuthenticator&) = delete;
    FaceAuthenticator(FaceAuthenticator&& other) noexcept;
    FaceAuthenticator& operator=(FaceAuthenticator&& other) noexcept;

    /**
     * Connect to device using the given serial config
     * Reconnect if already connected.
     *
     * @param[in] config Serial configuration
     * @return Status (Status::Ok on success).
     */
    Status Connect(const SerialConfig& config);

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
     * @return Status (Status::Ok on success).
     */
    Status Pair(const char* ecdsa_host_pubKey, const char* ecdsa_host_pubkey_sig, char* ecdsa_device_pubkey);

    /**
     * Unpair host with connected device.
     * This will disable security.
     *
     * @return Status (Status::Ok on success).
     */
    Status Unpair();
#endif // RSID_SECURE


    /**
     * Enroll a user.
     * Starts the enrollment process, which starts the camera, captures frames and then extracts
     * facial information at different poses.
     * During the process callbacks will be called to provide information if needed.
     * Once process is done, camera will be closed properly and device will be in ready state.
     *
     * @param[in] callback User defined callback to handle the process updates.
     * @param[in] user_id Null terminated C string of ascii chars. Max user id size is MAX_USERID_LENGTH bytes
     * @return Status (Status::Ok on success).
     */
    Status Enroll(EnrollmentCallback& callback, const char* user_id);

    /**
     * Enroll a user using an image of his face.
     * Note: The face should occupy at least 75% of image area
     * @param[in] user_id Null terminated C string of ascii chars. Max user id size is MAX_USERID_LENGTH bytes
     * @param[in] buffer bgr24 image buffer of the enrolled user face.
     * @param[in] width image width.
     * @param[in] height image height.
     * @return EnrollStatus (EnrollStatus::Success on success).
     */
    EnrollStatus EnrollImage(const char* user_id, const unsigned char* buffer, unsigned int width, unsigned int height);

    /**
     * Enroll a user using a normalized cropped image of his face.
     * Note: The image should be provided only by our face detection and image normalization code (see reference in our sample applications)
     * Important: This API clears the DB.
     * @param[in] user_id Null terminated C string of ascii chars. Max user id size is MAX_USERID_LENGTH bytes
     * @param[in] buffer bgr24 144x144 image buffer of the enrolled user
     * @return EnrollStatus (EnrollStatus::Success on success).
     */
    EnrollStatus EnrollCroppedFaceImage(const char* user_id, const unsigned char* buffer);

    /**
     * Extract features from RGB image.
     * Note: The face should occupy at least 75% of image area
     * @param[in] user_id Null terminated C string of ascii chars. Max user id size is MAX_USERID_LENGTH bytes
     * @param[in] buffer bgr24 image buffer of the enrolled user face. Max buffer size is 900KB(i.e. Width x Height x 3)
     * @param[in] width image width.
     * @param[in] height image height.
     * @param[out] pExtractedFaceprints the extracted faceprints from the image.
     * @return EnrollStatus (EnrollStatus::Success on success).
     */
    EnrollStatus EnrollImageFeatureExtraction(const char* user_id, const unsigned char* buffer, unsigned int width, unsigned int height,
                                              ExtractedFaceprints* pExtractedFaceprints);

    /**
     * Attempt to authenticate.
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
     * Starts infinite authentication loop. Call Cancel to stop it.
     *
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
     * @param[in] user_id Unique id (null terminated ascii, 31 bytes max) of the user that should be removed.
     * @return Status (Status::Ok on success).
     */
    Status RemoveUser(const char* user_id);

    /**
     * Attempt to remove all users from the device.
     *
     * @return Status (Status::Ok on success).
     */
    Status RemoveAll();

    /**
     * Apply authentication settings to FW.
     *
     * @param[in] device_config config with settings.
     * @return Status (Status::Ok on success).
     */
    Status SetDeviceConfig(const DeviceConfig& device_config);

    /**
     * Query FW authentication settings.
     *
     * @param[out] device_config config with settings.
     * @return Status (Status::Ok on success).
     */
    Status QueryDeviceConfig(DeviceConfig& device_config);

    /**
     * Query the device about all enrolled users.
     *
     * @param[out] user_ids pre-allocated array of user ids. app is expected to allocated array of length = QueryNumberOfUsers(),
     * each entry in the array is string of size = MAX_USERID_LENGTH
     * @param[in/out] number_of_users_in_out number of users to retrieve.
     * @return Status (Status::Ok on success).
     */
    Status QueryUserIds(char** user_ids, unsigned int& number_of_users_in_out);

    /**
     * Query the device about the number of enrolled users.
     *
     * @param[out] number_of_users number of users.
     * @return Status (Status::Ok on success).
     */
    Status QueryNumberOfUsers(unsigned int& number_of_users);

    /**
     * Send device to standby for lower power consumption - will auto wake up upon any action.
     *
     * @return Status (Status::Ok on success).
     */
    Status Standby();

    /**
     * Send device to hibernate. Lowest power consumption. Will wake up only upon GPIO 10 signal or power reset.
     *
     * @return Status (Status::Ok on success).
     */
    Status Hibernate();


    /**
     * Unlock the device that was locked due to too many spoof attempts.
     *
     * @return Status (Status::Ok on success).
     */
    Status Unlock();

    /**************************************************************************/
    /*************************** Host Mode Methods ****************************/
    /**************************************************************************/

    /**
     * Attempt to extract faceprints using enrollment flow.
     * Starts the enrollment process, which starts the camera, captures frames, extracts
     * faceprints of different face-poses, and sends them to the host.
     * During the process callbacks will be called to provide information if needed.
     * Once process is done, camera will be closed properly and device will be in ready state.
     *
     * @param[in] callback User defined callback to handle the process updates.
     * @return Status (Status::Ok on success).
     */
    Status ExtractFaceprintsForEnroll(EnrollFaceprintsExtractionCallback& callback);

    /**
     * Attempt to extract faceprints using authentication flow.
     * Starts the authentication procedure, which starts the camera, captures frames, extracts faceprints,
     * and sends them to the host.
     * During the process callbacks will be called to provide information if needed.
     * Once process is done, camera will be closed properly and device will be in ready state.
     *
     * @param[in] callback User defined callback object to handle the process updates.
     * @return Status (Status::Ok on success).
     */
    Status ExtractFaceprintsForAuth(AuthFaceprintsExtractionCallback& callback);

    /**
     * Attempt faceprints extraction in a loop using authentication flow.
     * Starts infinite authentication loop. Call Cancel to stop it.
     *
     * @param[in] callback User defined callback object to handle the process updates.
     * @return Status (Status::Ok on success).
     */
    Status ExtractFaceprintsForAuthLoop(AuthFaceprintsExtractionCallback& callback);

    /**
     * Match two faceprints to each other.
     * Calculates a score for how similar the two faceprints are, and returns a prediction for whether the
     * two faceprints belong to the same person.
     *
     * @param[in] new_faceprints faceprints which were extracted from a single image of a person.
     * @param[in] existing_faceprints faceprints which were calculated from one or more images of the same person.
     * @param[in] updated_faceprints a placeholder to write the updated-faceprints into, if the match was successful.
     * @param matcher_confidence_level confidence level
     * @return MatchResultHost match result, the 'success' field indicates if the two faceprints belong to the same
     * person.
     */
    MatchResultHost MatchFaceprints(MatchElement& new_faceprints, Faceprints& existing_faceprints, Faceprints& updated_faceprints,
                                    ThresholdsConfidenceEnum matcher_confidence_level);

    /**
     * Get the features descriptor for each user in the device's DB.
     * Number of users pulled is returned through num_of_users.
     *
     * @param[out] user_features pre-allocated array. Allocate this by calling QueryNumberOfUsers().
     * @param[out] num_of_users Number of users exported from the device.
     * @return Status (Status::Ok on success).
     */
    Status GetUsersFaceprints(Faceprints* user_features, unsigned int& num_of_users);

    /**
     * Insert each user entry from the array into the device's database.
     * @param[in] user_features Array of user IDs and feature descriptors.
     * @param[in] num_of_users Number of users in the array.
     * @return Status (Status::Ok on success).
     */
    Status SetUsersFaceprints(UserFaceprints* user_features, unsigned int num_of_users);

private:
    Impl::IFaceAuthenticator* _impl = nullptr;
};
} // namespace RealSenseID
