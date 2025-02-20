// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "AuthenticateStatus.h"
#include "FaceRect.h"
#include <vector>
#include "Faceprints.h"

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
     * Called to inform the client on the result of faceprints extraction, and pass the faceprints in case of success
     *
     * @param[in] status Final authentication status.
     * @param[in] faceprints Pointer to the requested faceprints which were just extracted from the device.
     */
    virtual void OnResult(const AuthenticateStatus status, const ExtractedFaceprints* faceprints) = 0;

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
    virtual void OnFaceDetected(const std::vector<FaceRect>& /*faces*/, const unsigned int /*ts*/)
    {
        // default empty impl for backward compatibilty
    }
};

} // namespace RealSenseID
