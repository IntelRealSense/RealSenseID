// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "FacePose.h"
#include "FaceRect.h"
#include "EnrollStatus.h"
#include <vector>

namespace RealSenseID
{
/**
 * User defined callback for enrollment.
 * Callback will be used to provide feedback to the client.
 */
class RSID_API EnrollmentCallback
{
public:
    virtual ~EnrollmentCallback() = default;

    /**
     * Called to inform the client that enroll operation ended.
     *
     * @param[in] status Final enroll status.
     */
    virtual void OnResult(const EnrollStatus status) = 0;

    /**
     * Called to inform the client whenever progress to enrollment has been made.
     *
     * @param[in] pose Current pose detected.
     */
    virtual void OnProgress(const FacePose pose) = 0;

    /**
     * Called to inform the client of problems encountered during the enrollment operation.
     *
     * @param[in] hint Hint for the problem encountered.
     */
    virtual void OnHint(const EnrollStatus hint) = 0;

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
