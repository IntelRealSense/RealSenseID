// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "AuthenticateStatus.h"
#include "FaceRect.h"
#include <vector>

namespace RealSenseID
{
/**
 * User defined callback for authentication.
 * Callback will be used to provide feedback to the client.
 */
class RSID_API AuthenticationCallback
{
public:
    virtual ~AuthenticationCallback() = default;

    /**
     * Called to inform the client that authentication operation ended.
     *
     * @param[in] status Final authentication status.
     * @param[in] userId Unique id (31 bytes) of the authenticated user if succeeded. Should be ignored in case of a
     * failure.
     */
    virtual void OnResult(const AuthenticateStatus status, const char* userId) = 0;

    /**
     * Called to inform the client of problems encountered during the authentication operation.
     *
     * @param[in] hint Hint for the problem encountered.
     */
    virtual void OnHint(const AuthenticateStatus hint) = 0;


    /**
     * Called to inform the client about detected faces during the authentication operation.
     *
     * @param[in] face Detected faces. First item is the selected one for the authentication operation.
     */
    virtual void OnFaceDetected(const std::vector<FaceRect>& /* faces */, const unsigned int /* ts */)
    {
        // default empty impl for backward compatibilty
    }
};
} // namespace RealSenseID
