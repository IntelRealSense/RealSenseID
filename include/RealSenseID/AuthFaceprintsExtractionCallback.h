// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

namespace RealSenseID
{

/**
 * User defined callback for faceprints extraction.
 * Callback will be used to provide feedback to the client.
 */
class AuthFaceprintsExtractionCallback
{
public:
    virtual ~AuthFaceprintsExtractionCallback() = default;

    /**
     * Called to inform the client that faceprints extraction operation ended.
     *
     * @param[in] status Final authentication status.
     * @param[in] userId Unique id (16 bytes) of the authenticated user if succeeded. Should be ignored in case of a
     * failure.
     */
    virtual void OnResult(const AuthenticateStatus status) = 0; // TODO : update

    /**
     * Called to inform the client of problems encountered during the authentication operation.
     *
     * @param[in] hint Hint for the problem encountered.
     */
    virtual void OnHint(const AuthenticateStatus hint) = 0;
};

} // namespace RealSenseID
