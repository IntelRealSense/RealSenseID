/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2011-2020 Intel Corporation. All Rights Reserved.
*******************************************************************************/
#pragma once

#include "RealSenseID/Faceprints.h"

namespace RealSenseID
{
/**
 * Face biometrics interface.
 * Will be used to provide extracted face faceprints from FaceBiometrics operations.
 */
class ExtendedFaceprints
{
public:
	char user_id[16]; // user id with null char
	Faceprints faceprints;    
};
} // namespace RealSenseID