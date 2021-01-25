// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/RealSenseIDExports.h"
#include "RealSenseID/AuthenticationCallback.h"
#include "RealSenseID/EnrollmentCallback.h"
#include "RealSenseID/SerialConfig.h"
#include "RealSenseID/Status.h"
#include "RealSenseID/SignatureCallback.h"
#include "RealSenseID/AuthConfig.h"

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
    explicit FaceAuthenticator(SignatureCallback* callback);
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

    /**
     * Disconnect from the device.
     */
    void Disconnect();

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
     * Enroll a user.
     *
     * Starts the enrollment process, which starts the camera, captures frames and then extracts
     * facial information at different poses.
     * During the process callbacks will be called to provide information if needed.
     * Once process is done, camera will be closed properly and device will be in ready state.
     *
     * @param[in] callback User defined callback to handle the process updates.
     * @param[in] user_id Null terminated C string of ascii chars. Max user id size is 15 bytes (max total of 16 bytes including the terminating zero byte).
     * user.
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
     * @param[in] authentication config with settings.
     * @return Status::Ok on success.
     */
    Status SetAuthSettings(const RealSenseID::AuthConfig& authConfig);

	/**
     * Query advanced settings to FW.
     * @param[out] authentication config with settings.
     * @return Status::Ok on success.
     */
    Status QueryAuthSettings(RealSenseID::AuthConfig& auth_config);
	
	/**
     * Query the device about all enrolled users.
     * @param[out] pre-allocated array of user ids.
	 * @param[in/out] number of users to retrieve.
     * @return Status::Ok on success.
     */
    Status QueryUserIds(char** user_ids, unsigned int& number_of_users);
	
	/**
     * Query the device about the number of enrolled users.
	 * @param[out] number of users.
     * @return Status::Ok on success.
     */
    Status QueryNumberOfUsers(unsigned int& number_of_users);
	
	  /**
     * Prepare device to stadby - for now it's saving database of users to flash.
     * @return Status::Ok on success.
     */
    Status Standby();

private:
    FaceAuthenticatorImpl* _impl = nullptr;
};
} // namespace RealSenseID
