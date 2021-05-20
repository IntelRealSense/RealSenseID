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
static constexpr int FACE_FACEPRINTS_VERSION = 7;

// values here aligned with those in MatcherImplDefines.h:
static constexpr size_t NUM_OF_RECOGNITION_FEATURES = 256;
// 3 extra elements (1 for hasMask , 1 for norm, 1 spare).
static constexpr size_t FEATURES_VECTOR_ALLOC_SIZE = 259;
static constexpr size_t HAS_MASK_INDEX_IN_FEATURS_VECTOR = 256;


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
// NOTES - (1) this structure must be aligned with struct SecureVersionDescriptor!
// order and types matters.
// (2) enrollment vector must be last member due to assumption/optimization made in OnFeaturesExtracted().
class Faceprints
{
public:

	int reserved[5]; // reserved placeholders (to minimize chance to re-create DB).

    int version = FACE_FACEPRINTS_VERSION;
    FaceprintsTypeEnum featuresType = W10;

	// flags - generic flags to indicate whatever we need.
	int flags= 0;

	// enrollmentDescriptor - is the enrollment faceprints per user.
    // adaptiveDescriptorWithoutMask - is the ongoing faceprints per user with mask (we update it over time).    
	// adaptiveDescriptorWithMask - is the ongoing faceprints per user with mask (we update it over time).    
	feature_t adaptiveDescriptorWithoutMask[FEATURES_VECTOR_ALLOC_SIZE];
    feature_t adaptiveDescriptorWithMask[FEATURES_VECTOR_ALLOC_SIZE];
	feature_t enrollmentDescriptor[FEATURES_VECTOR_ALLOC_SIZE];
};
} // namespace RealSenseID