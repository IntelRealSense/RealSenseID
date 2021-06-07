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

struct ExtendedMatchResult
{
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

// for adaptive-learning w/wo mask we adjust the thresholds to some configuration
// w.r.t. the 2 vectors been matched. Naming convention here:
//      p - probe vector, g - galery vector.
//      M - with mask, NM - no-mask.
// so for example _pM_gNM means we're going to match 
// a probe vector with mask VS. galery vector without mask.
typedef enum AdjustableThresholdsConfig : short
{
	ThresoldConfig_pNM_gNM = 0,
	ThresoldConfig_pM_gNM = 1,
	ThresoldConfig_pM_gM = 2,
	NumThresholdConfigs
} ThresholdsConfigEnum;

struct Thresholds
{   
    // naming convention here :
    //
    // M = with mask, NM = no mask (e.g. without mask)
    // p - prob, g - gallery (we match prob vector against gallery vector) 
    //
    // So for example:
    //
    // strongThreshold_pMgNM - refers to a prob vector with-mask 
    //                         against gallery vector without-mask.
    //
    match_calc_t identicalThreshold_gNMgNM;     // gallery no-mask, gallery no-mask.
    match_calc_t identicalThreshold_gMgNM;      // gallery mask, gallery no-mask.
    
    match_calc_t strongThreshold_pNMgNM;        // prob no-mask, gallery no-mask.
    match_calc_t strongThreshold_pMgM;          // prob with mask, gallery with mask.
    match_calc_t strongThreshold_pMgNM;         // prob with mask, gallery no-mask.

    // update thresholds.
    match_calc_t updateThreshold_pNMgNM;        
    match_calc_t updateThreshold_pMgM;          
    match_calc_t updateThreshold_pMgNM_First; // for opening first adaptive with-mask vector.
    
    // these are the active thresholds i.e. they will be set during runtime 
    // given the matched faceprints pair (w/wo mask etc).
    ThresholdsConfigEnum activeConfig;  
    match_calc_t activeIdenticalThreshold;
    match_calc_t activeStrongThreshold;
    match_calc_t activeUpdateThreshold;
};

class Matcher
{
public:
    // match single vs. single faceprints. Returns updated faceprints if update conditions fulfilled (indicated in result.should_update).
    static MatchResultInternal MatchFaceprints(const MatchElement& probe_faceprints, const Faceprints& existing_faceprints, Faceprints& updated_faceprints);

    // match single vs. an array of faceprints. Used e.g. when matching user against a set of users in the database.
    // returns updated faceprints if update conditions fulfilled (indicated in result.should_update). 
    // internal thresholds will be used.
    static ExtendedMatchResult MatchFaceprintsToArray(const MatchElement& probe_faceprints,
                                                      const std::vector<UserFaceprints_t>& existing_faceprints_array,
                                                      Faceprints& updated_faceprints);

    // match single vs. an array of faceprints. Used e.g. when matching user against a set of users in the database.
    // returns updated faceprints if update conditions fulfilled (indicated in result.should_update). 
    // thresholds provided by caller.
    static ExtendedMatchResult MatchFaceprintsToArray(const MatchElement& probe_faceprints,
                                                      const std::vector<UserFaceprints_t>& existing_faceprints_array,
                                                      Faceprints& updated_faceprints, Thresholds& thresholds);

    
    // checks the faceprints vector coordinates are in valid range [-1023,+1023]. 
    // if check_enrollment_vector=false it validates the adaptive faceprints, otherwise it validates the enrollment faceprints.
    static bool ValidateFaceprints(const Faceprints& faceprints, bool check_enrollment_vector=false);
    
    static bool ValidateFaceprints(const MatchElement& faceprints);
    
private:
    static void MatchTwoVectors(const feature_t* T1, const feature_t* T2, match_calc_t* match_score,
                                const uint32_t vec_length = RSID_NUM_OF_RECOGNITION_FEATURES);

    static void BlendAverageVector(feature_t* user_adaptive_faceprints, const feature_t* user_probe_faceprints,
                                   const uint32_t vec_length = RSID_NUM_OF_RECOGNITION_FEATURES);

    static bool LimitAdaptiveVector(feature_t* adaptive_faceprints_vec, const feature_t* anchor_faceprints_vec,
                                    const Thresholds& thresholds, const uint32_t vec_length = RSID_NUM_OF_RECOGNITION_FEATURES);
                                    

    static Thresholds GetDefaultThresholds();

    static void HandleThresholdsConfiguration(const bool& probe_has_mask,
                                                const Faceprints& existing_faceprints, 
                                                Thresholds& thresholds);
    static short GetMsb(const uint32_t ux);

    static void FaceMatch(const MatchElement& probe_faceprints,
                            const std::vector<UserFaceprints_t>& existing_faceprints_array, ExtendedMatchResult& result,
                            const bool& probe_has_mask);

    static bool GetScores(const MatchElement& probe_faceprints,
                          const std::vector<UserFaceprints_t>& existing_faceprints_array, 
                          TagResult& result, const bool& probe_has_mask);

    static match_calc_t CalculateConfidence(match_calc_t score);

    static bool ValidateVector(const feature_t* T1, const uint32_t vec_length = RSID_NUM_OF_RECOGNITION_FEATURES);

};

} // namespace RealSenseID
