// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once
#include "RealSenseID/MatchResult.h"
#include "RealSenseID/Faceprints.h"

namespace RealSenseID
{
class Matcher
{
public:
    static MatchResult MatchFaceprints(const Faceprints& new_faceprints, const Faceprints& existing_faceprints,
                                       Faceprints& updated_faceprints);
};

} // namespace RealSenseID
