// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once
#include "MatcherImplDefines.h"
#include "RealSenseID/Faceprints.h"
#include <vector>
#include <stdint.h>

namespace RealSenseID
{
using feature_t = short;
using match_calc_t = short;

class ExtendedFaceprints;

struct ExtendedMatchResult
{
    bool isIdentical = false;
    bool isSame = false;
    int userId = -1;
    match_calc_t maxScore = 0;
    match_calc_t confidence = 0;
};

struct MatchResultInternal
{
    bool success = false;
    bool should_update = false;
    match_calc_t confidence = 0;
    match_calc_t score = 0;
};

struct TagResult
{
    int idx = -1;
    int id = -1;
    match_calc_t score = 0;
    match_calc_t similarityScore = 0;
};

struct Thresholds
{
    match_calc_t identicalPersonThreshold;
    match_calc_t strongThreshold;
    match_calc_t updateThreshold;
};

class Matcher
{
public:
    static MatchResultInternal MatchFaceprints(const Faceprints& new_faceprints, const Faceprints& existing_faceprints,
                                               Faceprints& updated_faceprints);

    static ExtendedMatchResult MatchFaceprintsToArray(const Faceprints& new_faceprints,
                                                      const std::vector<ExtendedFaceprints>& existing_faceprints_array,
                                                      Faceprints& updated_faceprints);

    static ExtendedMatchResult MatchFaceprintsToArray(const Faceprints& new_faceprints,
                                                      const std::vector<ExtendedFaceprints>& existing_faceprints_array,
                                                      Faceprints& updated_faceprints, Thresholds thresholds);

    static void MatchFaceprintsToFaceprints(feature_t* T1, feature_t* T2, match_calc_t* retprob);

    static bool ValidateFaceprints(const Faceprints& faceprints);

    static Thresholds GetDefaultThresholds();

private:
    static short GetMsb(const uint32_t ux);

    static void FaceMatch(const Faceprints& new_faceprints,
                          const std::vector<ExtendedFaceprints>& existing_faceprints_array, ExtendedMatchResult& result,
                          Thresholds& thresholds);

    static bool GetScores(const Faceprints& new_faceprints,
                          const std::vector<ExtendedFaceprints>& existing_faceprints_array, TagResult& result,
                          match_calc_t threshold);

    static match_calc_t CalculateConfidence(match_calc_t score, match_calc_t threshold, ExtendedMatchResult& result);
};

} // namespace RealSenseID
