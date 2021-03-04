// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
#pragma once

static constexpr int RSID_NUMBER_OF_RECOGNITION_FACEPRINTS = 256;
static constexpr int RSID_FACE_FACEPRINTS_VERSION = 1;

namespace RealSenseID
{
/**
 * Face biometrics interface.
 * Will be used to provide extracted face faceprints from FaceBiometrics operations.
 */
class Faceprints
{
public:
    int version = RSID_FACE_FACEPRINTS_VERSION;
    int numberOfDescriptors = 0;
    short avgDescriptor[RSID_NUMBER_OF_RECOGNITION_FACEPRINTS];
};
} // namespace RealSenseID