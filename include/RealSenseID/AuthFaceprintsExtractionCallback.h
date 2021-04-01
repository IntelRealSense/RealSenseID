// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

namespace RealSenseID
{
class Faceprints;

/**
 * User defined callback for faceprints extraction.
 * Callback will be used to provide feedback to the client.
 */
class AuthFaceprintsExtractionCallback
{
public:
    virtual ~AuthFaceprintsExtractionCallback() = default;

    /**
     * Called to inform the client on the result of faceprints extraction, and pass the faceprints in case of success
     *
     * @param[in] status Final authentication status.
     * @param[in] faceprints Pointer to the requested faceprints which were just extracted from the device.     
     */
    virtual void OnResult(const AuthenticateStatus status, const Faceprints* faceprints) = 0;

    /**
     * Called to inform the client of problems encountered during the authentication operation.
     *
     * @param[in] hint Hint for the problem encountered.
     */
    virtual void OnHint(const AuthenticateStatus hint) = 0;
};

} // namespace RealSenseID
