// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "EnrollStatus.h"
#include "FacePose.h"

namespace RealSenseID
{
class Faceprints;

/**
 * User defined callback for faceprints extraction.
 * Callback will be used to provide feedback to the client.
 */
class EnrollFaceprintsExtractionCallback
{
public:
    virtual ~EnrollFaceprintsExtractionCallback() = default;

    /**
     * Called to inform the client that enroll operation ended.
     *
     * @param[in] status Final enroll status.
     * @param[in] faceprints Pointer to the requested faceprints which were just extracted from the device
     */
    virtual void OnResult(const EnrollStatus status, const Faceprints* faceprints) = 0;

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
};

} // namespace RealSenseID
