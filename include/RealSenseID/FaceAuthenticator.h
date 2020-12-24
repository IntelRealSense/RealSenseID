// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "RealSenseID/RealSenseIDExports.h"
#include "RealSenseID/AuthenticationCallback.h"
#include "RealSenseID/EnrollmentCallback.h"
#include "RealSenseID/SerialConfig.h"
#include "RealSenseID/AuthenticateStatus.h"
#include "RealSenseID/SerialStatus.h"
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
     * reconnect if already connected.
     *
     *  @param[in] config serial configuration
     * @return connection status
     */
    SerialStatus Connect(const SerialConfig& config);


    /**
     * Disconnect from the device.
     */
    void Disconnect();

    /**
     * Enroll a user.
     *
     * Starts the enrollment process, which starts the camera, captures frames and then extracts
     * facial information at different poses.
     * During the process callbacks will be called to provide information if needed.
     * Once process is done, camera will be closed properly and device will be in ready state.
     *
     * @param[in] callback User defined callback to handle the process updates.
     * @param[in] user_id Unique id (null terminated ascii, 16 bytes max) that will be given to the current enrolled
     * user.
     * @return EnrollStatus enroll result for this operation.
     */
    EnrollStatus Enroll(EnrollmentCallback& callback, const char* user_id);

    /**
     * Attempt to authenticate.
     *
     * Starts the authentication procedure, which starts the camera, captures frames and tries to match
     * the user in front of the camera to the enrolled users.
     * During the process callbacks will be called to provide information if needed.
     * Once process is done, camera will be closed properly and device will be in ready state.
     *
     * @param[in] callback User defined callback object to handle the process updates.
     * @return AuthenticateStatus authentication result for this operation.
     */
    AuthenticateStatus Authenticate(AuthenticationCallback& callback);

    /**
     * Start Authentication Loop.
     *
     * Starts infinite authentication loop. Call Cancel to stop it.
     * @param[in] callback User defined callback object to handle the process updates.
     * @return AuthenticateStatus Last authentication result for this operation.
     */
    AuthenticateStatus AuthenticateLoop(AuthenticationCallback& callback);

    /**
     * Cancel currently running operation.
     *
     * @return SerialStatus (SerialStatus::Success on success).
     */
    SerialStatus Cancel();


    /**
     * Attempt to remove specific user from the device.
     *
     * @param[in] user_id Unique id (null terminated ascii, 16 bytes max) of the user that should be removed.
     * @return SerialStatus::Success on success.
     */
    SerialStatus RemoveUser(const char* user_id);

    /**
     * Attempt to remove all users from the device.
     *
     * @return SerialStatus::Success on success.
     */
    SerialStatus RemoveAll();


    /**
     * Apply advanced settings to FW.
     * @param[in] authentication config with settings.
     * @return SerialStatus::Success on success.
     */
    SerialStatus SetAuthSettings(const RealSenseID::AuthConfig& authConfig);

private:
    RealSenseID::FaceAuthenticatorImpl* _impl = nullptr;
};
} // namespace RealSenseID
