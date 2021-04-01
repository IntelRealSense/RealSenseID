// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once
#include "RealSenseID/Faceprints.h"
#include <vector>

namespace RealSenseID
{
class ExtendedFaceprints;

typedef short FEATURE_TYPE;

struct ExtendedMatchResult
{
    bool isIdentical = false;
    bool isSame = false;
    bool isSimilar = false;
    int userId = -1;
    float maxScore = 0;
    float confidence = 0;
};

struct TagResult
{
    int idx = -1;
    int id = -1;
    float score = 0;
    float similarityScore = 0;
};

struct Thresholds
{
    float strongThreshold;
    float samePersonThreshold;
    float weakThreshold;
    float identicalPersonThreshold;
    float updateThreshold;
};

struct MatchResultInternal
{
    bool success = false;
    bool should_update = false;    
};

class Matcher
{
public:        
    static MatchResultInternal MatchFaceprints(const Faceprints& new_faceprints, const Faceprints& existing_faceprints, Faceprints& updated_faceprints);    

    static ExtendedMatchResult MatchFaceprintsToArray(const Faceprints& new_faceprints, const std::vector<ExtendedFaceprints>& existing_faceprints_array, Faceprints& updated_faceprints);
    static ExtendedMatchResult MatchFaceprintsToArray(const Faceprints& new_faceprints, const std::vector<ExtendedFaceprints>& existing_faceprints_array, Faceprints& updated_faceprints, Thresholds thresholds);
    static void MatchFaceprintsToFaceprints(FEATURE_TYPE* T1, FEATURE_TYPE* T2, float* retprob);
    static bool ValidateFaceprints(const Faceprints& faceprints);
    static Thresholds GetDefaultThresholds();

private:
    static void FaceMatch(const Faceprints& new_faceprints, const std::vector<ExtendedFaceprints>& existing_faceprints_array, ExtendedMatchResult& result, Thresholds& thresholds);
    static void GetScores(const Faceprints& new_faceprints, const std::vector<ExtendedFaceprints>& existing_faceprints_array, TagResult& result, float threshold);    
};

} // namespace RealSenseID
