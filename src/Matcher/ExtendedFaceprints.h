// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.
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
	char user_id[31]; // user id with null char
	Faceprints faceprints;    
};
} // namespace RealSenseID