/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2011-2020 Intel Corporation. All Rights Reserved.
*******************************************************************************/
#pragma once

#include <cstddef>
#include <cstdint> 

namespace RealSenseID
{
static constexpr size_t NUMBER_OF_RECOGNITION_FACEPRINTS = 256;
static constexpr size_t FEATURES_VECTOR_ALLOC_SIZE = 256;

static constexpr int FACE_FACEPRINTS_VERSION = 4;

using feature_t = short;

// placeholder for future (RGB feature extraction).
typedef enum FaceprintsType : uint16_t
{
    W10 = 0,
    RGB
} FaceprintsTypeEnum;

/**
 * Face biometrics interface.
 * Will be used to provide extracted face faceprints from FaceBiometrics operations.
 */
class Faceprints
{
public:
    int version = FACE_FACEPRINTS_VERSION;
    int numberOfDescriptors = 0;
    FaceprintsTypeEnum featuresType = W10;
    // origDescriptor - is the enrollment faceprints per user.
    // avgDescriptor - is the ongoing faceprints per user (we update it over time).    
    feature_t avgDescriptor[FEATURES_VECTOR_ALLOC_SIZE];
    feature_t origDescriptor[FEATURES_VECTOR_ALLOC_SIZE];
};
} // namespace RealSenseID