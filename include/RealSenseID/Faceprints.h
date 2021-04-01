/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2011-2020 Intel Corporation. All Rights Reserved.
*******************************************************************************/
#pragma once

static constexpr int RSID_NUMBER_OF_RECOGNITION_FACEPRINTS = 256;
static constexpr int RSID_FACE_FACEPRINTS_VERSION = 2;

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