/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2011-2020 Intel Corporation. All Rights Reserved.
*******************************************************************************/
#ifndef __FACEPRINTSS__H___
#define __FACEPRINTSS__H___
// #pragma once
// pragma once ends with redefinition errors on jenkins.

#include <cstddef>
#include <cstdint>
#include <string>
#include "FaceprintsDefines.h"

namespace RealSenseID
{

static constexpr size_t RSID_MAX_USER_ID_LENGTH_IN_DB = 31;

// extracted faceprints element
// a reduced structure that is used to represent the extracted faceprints been transferred from the device to the host
// through the packet layer.
class ExtractedFaceprints
{
public:
    RealSenseID::ExtractedFaceprintsElement data;
};

// db layer faceprints element.
// a structure that is used in the DB layer, to save user DBFaceprintsElement plus additional metadata to the DB.
// the struct includes several vectors and metadata to support all our internal matching mechanism (e.g. adaptive-learning etc..).
class Faceprints
{
public:
    RealSenseID::DBFaceprintsElement data;
};

// match element used during authentication flow, where we match between faceprints object received from the device
// to user objects read from the DB.
class MatchElement
{
public:
    RealSenseID::ExtractedFaceprintsElement data;
};

// faceprints plus username element.
typedef struct UserFaceprints
{
    char user_id[RSID_MAX_USER_ID_LENGTH_IN_DB];
    Faceprints faceprints;
} UserFaceprints_t;

} // namespace RealSenseID

#endif // __FACEPRINTSS__H___