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
    bool should_update = false;
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
    // match single vs. single faceprints. Returns updated faceprints if update conditions fulfilled (indicated in result.should_update).
    static MatchResultInternal MatchFaceprints(const Faceprints& new_faceprints, const Faceprints& existing_faceprints, Faceprints& updated_faceprints);

    // match single vs. an array of faceprints. Used e.g. when matching user against a set of users in the database.
    // returns updated faceprints if update conditions fulfilled (indicated in result.should_update). 
    // internal thresholds will be used.
    static ExtendedMatchResult MatchFaceprintsToArray(const Faceprints& new_faceprints,
                                                      const std::vector<ExtendedFaceprints>& existing_faceprints_array,
                                                      Faceprints& updated_faceprints);

    // match single vs. an array of faceprints. Used e.g. when matching user against a set of users in the database.
    // returns updated faceprints if update conditions fulfilled (indicated in result.should_update). 
    // thresholds provided by caller.
    static ExtendedMatchResult MatchFaceprintsToArray(const Faceprints& new_faceprints,
                                                      const std::vector<ExtendedFaceprints>& existing_faceprints_array,
                                                      Faceprints& updated_faceprints, Thresholds thresholds);

    
    // checks the faceprints vector coordinates are in valid range [-1023,+1023]. 
    // if check_orig=false it validates the avg faceprints, otherwise it validates the orig faceprints.
    static bool ValidateFaceprints(const Faceprints& faceprints, bool check_orig=false);
    
    
private:
    static void MatchTwoVectors(const feature_t* T1, const feature_t* T2, match_calc_t* retprob,
                                const uint32_t vec_length = RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER);

    static void BlendAverageVector(feature_t* user_average_faceprints, const feature_t* user_new_faceprints,
                                   const uint32_t vec_length = RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER);

    static bool UpdateAverageVector(feature_t* updated_faceprints_vec, const feature_t* orig_faceprints_vec,
                                    const uint32_t vec_length = RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER);

    static Thresholds GetDefaultThresholds();

    static short GetMsb(const uint32_t ux);

    static void FaceMatch(const Faceprints& new_faceprints,
                          const std::vector<ExtendedFaceprints>& existing_faceprints_array, ExtendedMatchResult& result,
                          Thresholds& thresholds);

    static bool GetScores(const Faceprints& new_faceprints,
                          const std::vector<ExtendedFaceprints>& existing_faceprints_array, TagResult& result,
                          match_calc_t threshold);

    static match_calc_t CalculateConfidence(match_calc_t score, match_calc_t threshold, ExtendedMatchResult& result);

    static bool ValidateVector(const feature_t* T1, const uint32_t vec_length = RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER);

};

} // namespace RealSenseID
