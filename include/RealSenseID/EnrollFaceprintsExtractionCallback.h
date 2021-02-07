// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

namespace RealSenseID
{

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
};

} // namespace RealSenseID
