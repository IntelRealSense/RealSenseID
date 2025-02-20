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
#include <cstddef>
#include <string>

namespace RealSenseID
{
/**
 *  Max user id size is 30 bytes, 31 bytes including '\0'
 */
static constexpr size_t MAX_USERID_LENGTH = RSID_MAX_USER_ID_LENGTH_IN_DB;

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

     * including the terminating zero byte). user.
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
     * @param[out] the extracted faceprints from the image.
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
     * @param[out] pre-allocated array of user ids. app is expected to allocated array of length = QueryNumberOfUsers(),
     * each entry in the array is string of size = MAX_USERID_LENGTH
     * @param[in/out] number of users to retrieve.
     * @return Status (Status::Ok on success).
     */
    Status QueryUserIds(char** user_ids, unsigned int& number_of_users_in_out);

    /**
     * Query the device about the number of enrolled users.
     *
     * @param[out] number of users.
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
     * @return MatchResultHost match result, the 'success' field indicates if the two faceprints belong to the same
     * person.
     */
    MatchResultHost MatchFaceprints(
        MatchElement& new_faceprints, Faceprints& existing_faceprints, Faceprints& updated_faceprints,
        ThresholdsConfidenceEnum matcher_confidence_level = ThresholdsConfidenceEnum::ThresholdsConfidenceLevel_High);

    /**
     * Get the features descriptor for each user in the device's DB.
     * Number of users pulled is returned through num_of_users.
     *
     * @param[out] pre-acllocated array. Allocate this by calling QueryNumberOfUsers().
     * @param[out] Number of users exported from the device.
     * @return Status (Status::Ok on success).
     */
    Status GetUsersFaceprints(Faceprints* user_features, unsigned int& num_of_users);

    /**
     * Insert each user entry from the array into the device's database.
     * @param[in] Array of user IDs and feature descriptors.
     * @param[in] Number of users in the array.
     * @return Status (Status::Ok on success).
     */
    Status SetUsersFaceprints(UserFaceprints* user_features, unsigned int num_of_users);


    /**
     * Sets the license key
     *
     * Passing an empty string will undo the override, causing GetLicenseKey to read from the persisted value.
     *
     * @param[in] license_key The license key to set. Pass an empty string to revert to the persisted license key.
     * @return Status::Ok on successful setting of the license key, or an error status code indicating the failure
     * reason.
     */
    static Status SetLicenseKey(const std::string& license_key);


    /**
     * Returns the currently used license key.
     *
     * If SetLicenseKey has been called previously, this function will return the value set by it
     * until SetLicenseKey is called again with an empty string. If SetLicenseKey has not been
     * called or was called with an empty string, the license key will try read from storage if it exists.
     *
     * @return The current license key if found, or an empty string if not.
     */
    static std::string GetLicenseKey();


    /**
     * Initiates a license provision session between the device, host and the cloud-based license server.
     *
     * This operation may take a few seconds to complete, depending on network conditions.
     *
     * @return Status::Ok on successful completion of the license provision session, or an error status code indicating
     * the failure reason.
     */
    Status ProvideLicense();


    using OnStartLicenseSession = void (*)();
    using OnEndLicenseSession = void (*)(Status);

    /**
     * Enables automatic handling of license checks (enabled by default) using the cloud-based license server.
     *
     * For non-perpetual licenses, the device may respond with `Status::LicenseCheck` to indicate that the license needs
     * re-validation. In such cases, the host must perform a license validation session with the cloud-based license
     * server and the device.
     *
     * Note: License validation operations may take a few seconds to complete, depending on network conditions.
     *
     * Note: Perpetual licenses never require re-validation of the license and the operation will not be triggered.
     *
     * @param[in] enable Set to true to enable automatic handling, or false to disable it (true by default).
     * @param[in] on_start_license_session callback function that is called when a license validation operation begins
     * (nullptr by default).
     * @param[in] on_end_license_session callback function that is called when a license validation operation ends
     * (nullptr by default).
     */
    void EnableLicenseCheckHandler(OnStartLicenseSession on_start_license_session, OnEndLicenseSession on_end_license_session);

    /**
     * Disable automatic automatic handling of license check requests from the device.
     */
    void DisableLicenseCheckHandler();

private:
    FaceAuthenticatorImpl* _impl = nullptr;
    bool _enable_license_handler = true;
    OnStartLicenseSession _on_start_license_session = nullptr;
    OnEndLicenseSession _on_end_license_session = nullptr;

    // Perform license check with license server if given status is LicenseCheck and if _enable_license_handler is set
    // to true. Return true if license check was performed succesfully or false if it was not required or failed.
    template <typename T>
    bool HandleLicenseCheck(T status);
};
} // namespace RealSenseID
