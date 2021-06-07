// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "Matcher.h"
#include "Logger.h"
#include "RealSenseID/Faceprints.h"
#include <cmath>
#include <assert.h>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <algorithm>
// #include <iostream>

/*
RSID-MATCHER INFO: Functions implementation here in Matcher.cc uses integer arithmetic and tailored to integer-valued
feature-vectors with: (1) length 256 (2) integer valued features in range [-1023,+1023]. Adjustments and checks will be
taken if (1) vectors length becomes more than 256 (2) features value range becomes wider than [-1023,+1023].
RSID-MARCHER INFO: Such adjustments/checks may be required due to accumulators and bit shifts used.
*/

namespace RealSenseID
{
using match_calc_t = short; 

static const char* LOG_TAG = "Matcher";

static const match_calc_t s_maxFeatureValue = static_cast<match_calc_t>(RSID_MAX_FEATURE_VALUE);
static const match_calc_t s_minPossibleScore = static_cast<match_calc_t>(RSID_MIN_POSSIBLE_SCORE);

static const match_calc_t s_identicalThreshold_gNMgNM = static_cast<match_calc_t>(RSID_IDENTICAL_THRESHOLD_GNM_GNM);
static const match_calc_t s_identicalThreshold_gMgNM = static_cast<match_calc_t>(RSID_IDENTICAL_THRESHOLD_GM_GNM);

static const match_calc_t s_strongThreshold_pNMgNM = static_cast<match_calc_t>(RSID_STRONG_THRESHOLD_PNM_GNM);
static const match_calc_t s_strongThreshold_pMgM = static_cast<match_calc_t>(RSID_STRONG_THRESHOLD_PM_GM);
static const match_calc_t s_strongThreshold_pMgNM = static_cast<match_calc_t>(RSID_STRONG_THRESHOLD_PM_GNM);

static const match_calc_t s_updateThreshold_pNMgNM = static_cast<match_calc_t>(RSID_UPDATE_THRESHOLD_PNM_GNM);
static const match_calc_t s_updateThreshold_pMgM = static_cast<match_calc_t>(RSID_UPDATE_THRESHOLD_PM_GM);
static const match_calc_t s_updateThreshold_pMgNM_First = static_cast<match_calc_t>(RSID_UPDATE_THRESHOLD_PM_GNM_FIRST);

// for linear piecewise curve : score to confidence mapping.
static const int s_linCurveMultiplier1 = RSID_LIN1_CURVE_MULTIPLIER;
static const int s_linCurveSabtractive1 = RSID_LIN1_CURVE_SABTRACTIVE;
static const int s_linCurveAdditive1 = RSID_LIN1_CURVE_ADDITIVE;
static const int s_linCurveHeadroom1 = RSID_LIN1_CURVE_HR;

static const int s_linCurveMultiplier2 = RSID_LIN2_CURVE_MULTIPLIER;
static const int s_linCurveSabtractive2 = RSID_LIN2_CURVE_SABTRACTIVE;
static const int s_linCurveAdditive2 = RSID_LIN2_CURVE_ADDITIVE;
static const int s_linCurveHeadroom2 = RSID_LIN2_CURVE_HR;

static bool IsSameVersion(const Faceprints& newFaceprints, const Faceprints& existingFaceprints)
{
    bool versionsMatch = (newFaceprints.data.version == existingFaceprints.data.version);
    if (!versionsMatch)
    {
        LOG_ERROR(LOG_TAG, "Faceprints versions don't match");
    }
    return versionsMatch;
}

static bool IsSameVersion(const MatchElement& newFaceprints, const Faceprints& existingFaceprints)
{
    bool versionsMatch = (newFaceprints.data.version == existingFaceprints.data.version);
    if (!versionsMatch)
    {
        LOG_ERROR(LOG_TAG, "Faceprints versions don't match");
    }
    return versionsMatch;
}

void Matcher::HandleThresholdsConfiguration(const bool& probe_has_mask,
                        const Faceprints& existing_faceprints, 
                        Thresholds& thresholds)
{
    feature_t* galeryAdaptiveVector = nullptr;
    
    // here we handle with/without mask adaptive learning.
    // we adjust the correct thresholds and adaptiveVector for w/wo mask scenarios.
    if(!probe_has_mask)
    {
        thresholds.activeConfig = ThresholdsConfigEnum::ThresoldConfig_pNM_gNM;
        thresholds.activeIdenticalThreshold = thresholds.identicalThreshold_gNMgNM;
        thresholds.activeStrongThreshold = thresholds.strongThreshold_pNMgNM;
        thresholds.activeUpdateThreshold = thresholds.updateThreshold_pNMgNM;
    }
    else
    {            
        // does the withMask vector is valid ?
        feature_t vec_flags = existing_faceprints.data.adaptiveDescriptorWithMask[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS];
        bool is_valid = (vec_flags == FaVectorFlagsEnum::VecFlagValidWithMask) ? true : false;

        if(is_valid)
        {
            // apply adaptation on the WithMask[] vector
            thresholds.activeConfig = ThresholdsConfigEnum::ThresoldConfig_pM_gM;
            // if with-mask the anchor vector is the _gNM vector anyway, so identical threshold
            // is _pMgNM.
            thresholds.activeIdenticalThreshold = thresholds.identicalThreshold_gMgNM;
            thresholds.activeStrongThreshold = thresholds.strongThreshold_pMgM;
            thresholds.activeUpdateThreshold = thresholds.updateThreshold_pMgM;
        }
        else
        {
            // apply adaptation on the WithoutMask[] vector
            thresholds.activeConfig = ThresholdsConfigEnum::ThresoldConfig_pM_gNM;
            // if with-mask the anchor vector is the _gNM vector anyway, so identical threshold
            // is _pMgNM.
            thresholds.activeIdenticalThreshold = thresholds.identicalThreshold_gMgNM;  
            thresholds.activeStrongThreshold = thresholds.strongThreshold_pMgNM;
            thresholds.activeUpdateThreshold = thresholds.updateThreshold_pMgNM_First;
        }
    }

#if(ENABLE_MATCHER_DEBUG_LOGS)
    LOG_DEBUG(LOG_TAG, "----> Matcher active setup = %d : hasMask = %d, strongTH = %d, updateTH = %d, identicalTH = %d.", 
            thresholds.activeConfig, probe_has_mask, thresholds.activeStrongThreshold, 
            thresholds.activeUpdateThreshold, thresholds.activeIdenticalThreshold);
#endif

    return;
}

bool Matcher::GetScores(const MatchElement& probe_faceprints,
                        const std::vector<UserFaceprints_t>& existing_faceprints_array, 
                        TagResult& result, const bool& probe_has_mask)
{
    
    if (existing_faceprints_array.size() == 0)
    {
        LOG_ERROR(LOG_TAG, "Can't match with empty array.");
        return false;
    }

    result.score = 0;
    result.id = -1;
    
    match_calc_t maxScore = -1; // must init to -1 so that maximum will be saved if matchScore is 0 !!!
    match_calc_t matchScore = -1;
    int numberOfSubjects = (int)existing_faceprints_array.size();
    int maxSubject = -1;
    uint32_t vec_length = RSID_NUM_OF_RECOGNITION_FEATURES;

    const feature_t* probeVector = (feature_t*)(&(probe_faceprints.data.featuresVector[0]));
    feature_t* galeryAdaptiveVector = nullptr;

    for (int subjectIndex = 0; subjectIndex < numberOfSubjects; subjectIndex++)
    {
        matchScore = s_minPossibleScore;
        auto& existing_faceprints = existing_faceprints_array[subjectIndex];

        if (!ValidateFaceprints(existing_faceprints.faceprints))
        {
            LOG_ERROR(LOG_TAG, "Invalid faceprints vector range");
            return false;
        }

        if (!IsSameVersion(probe_faceprints, existing_faceprints.faceprints))
        {
            LOG_ERROR(LOG_TAG, "Mismatch in faceprints versions");
            return false;
        }

        // here we handle adaptive-learning for with/without mask vectors.
        // choose the correct adaptiveVector.
        if(!probe_has_mask)
        {
            galeryAdaptiveVector = (feature_t*)(&existing_faceprints.faceprints.data.adaptiveDescriptorWithoutMask[0]);
        }
        else
        {

            // does the adaptive-withMask vector is valid ?
            feature_t vec_flags = existing_faceprints.faceprints.data.adaptiveDescriptorWithMask[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS];
            bool is_valid = (vec_flags == FaVectorFlagsEnum::VecFlagValidWithMask) ? true : false;

            if(is_valid)
            {
                galeryAdaptiveVector = (feature_t*)(&existing_faceprints.faceprints.data.adaptiveDescriptorWithMask[0]);
            }
            else
            {
                galeryAdaptiveVector = (feature_t*)(&existing_faceprints.faceprints.data.adaptiveDescriptorWithoutMask[0]);
            }

        }

        MatchTwoVectors(probeVector, galeryAdaptiveVector, &matchScore, vec_length);

        // save max found so far
        if (matchScore > maxScore)
        {
            maxScore = matchScore;
            maxSubject = static_cast<int>(subjectIndex);
        }
    }

    result.score = maxScore;
    result.id = maxSubject;

    return true;
}

void Matcher::FaceMatch(const MatchElement& probe_faceprints,
                        const std::vector<UserFaceprints_t>& existing_faceprints_array, ExtendedMatchResult& result,
                        const bool& probe_has_mask)
{
    result.isSame = false;
    result.maxScore = 0;
    result.confidence = 0;
    result.userId = -1;
    result.should_update = false;

    TagResult scoresResult;
    // this function returns the index and info of the best score winner in the array.
    bool isScoreSuccess = GetScores(probe_faceprints, existing_faceprints_array,
                                    scoresResult, probe_has_mask);

    if (!isScoreSuccess)
    {
        LOG_ERROR(LOG_TAG, "Failed during GetScores() - please check.");
        return;
    }

    result.maxScore = scoresResult.score;
    result.userId = scoresResult.id;
    
    // don't set yet - we must call HandleThresholdsConfiguration() first!
    // result.isSame = (scoresResult.score > thresholds.activeStrongThreshold);
    
    result.confidence = CalculateConfidence(scoresResult.score);
}

static void SetToDefaultThresholds(Thresholds& thresholds)
{
    thresholds.identicalThreshold_gNMgNM = s_identicalThreshold_gNMgNM;
    thresholds.identicalThreshold_gMgNM = s_identicalThreshold_gMgNM;
    
    thresholds.strongThreshold_pNMgNM = s_strongThreshold_pNMgNM;
    thresholds.strongThreshold_pMgM = s_strongThreshold_pMgM;
    thresholds.strongThreshold_pMgNM = s_strongThreshold_pMgNM;
    
    thresholds.updateThreshold_pNMgNM = s_updateThreshold_pNMgNM;
    thresholds.updateThreshold_pMgM = s_updateThreshold_pMgM;
    thresholds.updateThreshold_pMgNM_First = s_updateThreshold_pMgNM_First;
}

bool Matcher::ValidateFaceprints(const Faceprints& faceprints, bool check_enrollment_vector)
{
    // TODO - carefull handling in MatchTwoVectors() may be required for vectors longer than 256.
    static_assert((RSID_NUM_OF_RECOGNITION_FEATURES <= 256),
                  "Vector length is higher than 256 - may need careful test and check, due to integer arithmetics and "
                  "overflow risk!");

    uint32_t nfeatures = static_cast<uint32_t>(RSID_NUM_OF_RECOGNITION_FEATURES);
    
    bool is_valid = true;

    if(check_enrollment_vector)
    {
        is_valid = ValidateVector(&faceprints.data.enrollmentDescriptor[0], nfeatures);
    }
    else
    {
        is_valid = ValidateVector(&faceprints.data.adaptiveDescriptorWithoutMask[0], nfeatures);
    }
    
    if (!is_valid)
    {
        LOG_ERROR(LOG_TAG, "Vector (faceprint) validation failed!");
    }

    return is_valid;
}

bool Matcher::ValidateFaceprints(const MatchElement& faceprints)
{
    // TODO - carefull handling in MatchTwoVectors() may be required for vectors longer than 256.
    static_assert((RSID_NUM_OF_RECOGNITION_FEATURES <= 256),
                  "Vector length is higher than 256 - may need careful test and check, due to integer arithmetics and "
                  "overflow risk!");

    uint32_t nfeatures = static_cast<uint32_t>(RSID_NUM_OF_RECOGNITION_FEATURES);
    
    bool is_valid = ValidateVector(&faceprints.data.featuresVector[0], nfeatures);

    if (!is_valid)
    {
        LOG_ERROR(LOG_TAG, "Vector (faceprint) validation failed!");
    }

    return is_valid;
}

ExtendedMatchResult Matcher::MatchFaceprintsToArray(const MatchElement& probe_faceprints,
                                                    const std::vector<UserFaceprints_t>& existing_faceprints_array,
                                                    Faceprints& updated_faceprints)
{
    Thresholds thresholds;
    SetToDefaultThresholds(thresholds);
    ExtendedMatchResult result;
    
    result.userId = -1;
    result.maxScore = 0;

    if (!ValidateFaceprints(probe_faceprints)) 
	{
        LOG_ERROR(LOG_TAG, "Faceprints vector failed range validation.");
		return result;	
	}
    
    if(existing_faceprints_array.size() <= 0)
    {
        LOG_ERROR(LOG_TAG, "Faceprints array size is 0.");
        return result;	
    }

    if (probe_faceprints.data.version != existing_faceprints_array[0].faceprints.data.version) 
    {
        LOG_ERROR(LOG_TAG, "version mismatch between 2 vectors. Skipping this match()!");
		return result;	
    }

    // Try to match the 2 faceprints, and raise should_update flag respecively.
    // note that here we also set the active thresholds configurtion  
    // which is used for adaptive-learning w/wo mask.
    feature_t probeFaceFlags = probe_faceprints.data.featuresVector[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS];
    bool probe_has_mask = (probeFaceFlags == FaVectorFlagsEnum::VecFlagValidWithMask) ? true : false;

    FaceMatch(probe_faceprints, existing_faceprints_array, result, probe_has_mask);

    size_t user_index = (size_t)result.userId;

    // if no user matched, finish here and return.
    if((user_index < 0) || (user_index >= existing_faceprints_array.size()))
    {
        LOG_ERROR(LOG_TAG, "Invalid user_index : Skipping function.");
        return result;
    }

    // here we handle with/without mask adaptive learning.
    // we choose the correct thresholds Configuration, based on the probe-vector and the (matched) gallery-vector.
    HandleThresholdsConfiguration(probe_has_mask, existing_faceprints_array[user_index].faceprints, thresholds);

    // here correct active thresholds set correctly, so we can use them.
    result.isSame = (result.maxScore > thresholds.activeStrongThreshold);
    result.should_update = (result.maxScore >= thresholds.activeUpdateThreshold) && result.isSame;

    // if should_update then we create an update vector such that:
    // (1) the current new vector is blended into the latest adaptive vector.
    // (2) then we make sure that the updated vector is not too far from the enrollment vector. 
    if(result.should_update)
    {
        // Init updated_faceprints to the faceprints already exists in the DB
        //  
        updated_faceprints = existing_faceprints_array[user_index].faceprints;

        const uint32_t vec_length = RSID_NUM_OF_RECOGNITION_FEATURES;
        const feature_t* probeVector = &probe_faceprints.data.featuresVector[0];
        const feature_t* anchorVector = nullptr;
        feature_t* galeryAdaptiveVector = nullptr;

        // handle with/without mask vectors properly.
        // choose the correct adaptive vector, and set its flags (based on thresholds configuration).
        switch(thresholds.activeConfig)
        {
            case ThresholdsConfigEnum::ThresoldConfig_pM_gNM:
                // since we are here only is should_update=true, then we 
                // set the adaptive withMask[] vector for the first time (with values of the new faceprints).
                //
                // since its the FIRST TIME - it will probably take 10-20 iterations to converge
                // during LimitAdaptiveVector().
                anchorVector = &updated_faceprints.data.adaptiveDescriptorWithoutMask[0];
                galeryAdaptiveVector = &updated_faceprints.data.adaptiveDescriptorWithMask[0];
                ::memcpy(galeryAdaptiveVector, probeVector, sizeof(probe_faceprints.data.featuresVector));
                // mark the vector as "valid with mask"
                galeryAdaptiveVector[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS] = FaVectorFlagsEnum::VecFlagValidWithMask;
#if(ENABLE_MATCHER_DEBUG_LOGS)
                LOG_DEBUG(LOG_TAG, "----> With-mask adaptation (first-time).");
#endif
                break;

            case ThresholdsConfigEnum::ThresoldConfig_pM_gM:
                anchorVector = &updated_faceprints.data.adaptiveDescriptorWithoutMask[0];
                galeryAdaptiveVector =  &updated_faceprints.data.adaptiveDescriptorWithMask[0];
                //galeryAdaptiveVector[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS] = FaVectorFlagsEnum::VecFlagValidWithMask;
#if(ENABLE_MATCHER_DEBUG_LOGS)
                LOG_DEBUG(LOG_TAG, "----> With-mask adaptation (not first-time).");
#endif
                break;

            case ThresholdsConfigEnum::ThresoldConfig_pNM_gNM:
            default:
                anchorVector = &updated_faceprints.data.enrollmentDescriptor[0];
                galeryAdaptiveVector = &updated_faceprints.data.adaptiveDescriptorWithoutMask[0];
                // galeryAdaptiveVector[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS] = FaVectorFlagsEnum::VecFlagValidWithoutMask;
#if(ENABLE_MATCHER_DEBUG_LOGS)
                LOG_DEBUG(LOG_TAG, "----> Without-mask adaptation.");
#endif
                break;
        }

        // blend the current adaptive galery vector with the new vector
        BlendAverageVector(galeryAdaptiveVector, probeVector, vec_length);
        
        // make sure blended adaptive vector is not too far from enrollment vector
        bool update_was_ok = LimitAdaptiveVector(galeryAdaptiveVector, anchorVector,
                                                thresholds, vec_length);

        // disable update flag if something went wrong in the update process.
        result.should_update = result.should_update && update_was_ok;
    }

    return result;
}

ExtendedMatchResult Matcher::MatchFaceprintsToArray(const MatchElement& probe_faceprints,
                                                    const std::vector<UserFaceprints_t>& existing_faceprints_array,
                                                    Faceprints& updated_faceprints, Thresholds& thresholds)
{    
    ExtendedMatchResult result;

    result.userId = -1;
    result.maxScore = 0;

    if (!ValidateFaceprints(probe_faceprints)) 
	{
        LOG_ERROR(LOG_TAG, "Faceprints vector failed range validation.");
		return result;	
	}
    
    if(existing_faceprints_array.size() <= 0)
    {
        LOG_ERROR(LOG_TAG, "Faceprints array size is 0.");
        return result;	
    }

    if (probe_faceprints.data.version != existing_faceprints_array[0].faceprints.data.version) 
    {
        LOG_ERROR(LOG_TAG, "version mismatch between 2 vectors. Skipping this match()!");
		return result;	
    }

    // Try to match the 2 faceprints, and raise should_update flag respecively.
    // note that here we also set the active thresholds configurtion  
    // which is used for adaptive-learning w/wo mask.
    feature_t probeFaceFlags = probe_faceprints.data.featuresVector[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS];
    bool probe_has_mask = (probeFaceFlags == FaVectorFlagsEnum::VecFlagValidWithMask) ? true : false;

    FaceMatch(probe_faceprints, existing_faceprints_array, result, probe_has_mask);

    size_t user_index = (size_t)result.userId;

    // if no user matched, finish here and return.
    if((user_index < 0) || (user_index >= existing_faceprints_array.size()))
    {
        LOG_ERROR(LOG_TAG, "Invalid user_index : Skipping function.");
        return result;
    }

    // here we handle with/without mask adaptive learning.
    // we choose the correct thresholds Configuration, based on the probe-vector and the (matched) gallery-vector.
    HandleThresholdsConfiguration(probe_has_mask, existing_faceprints_array[user_index].faceprints, thresholds);

    // here correct active thresholds set correctly, so we can use them.
    result.isSame = (result.maxScore > thresholds.activeStrongThreshold);
    result.should_update = (result.maxScore >= thresholds.activeUpdateThreshold) && result.isSame;

    // if should_update then we create an update vector such that:
    // (1) the current new vector is blended into the latest adaptive vector.
    // (2) then we make sure that the updated vector is not too far from the enrollment vector. 
    if(result.should_update)
    {
        // Init updated_faceprints to the faceprints already exists in the DB
        //  
        updated_faceprints = existing_faceprints_array[user_index].faceprints;

        const uint32_t vec_length = RSID_NUM_OF_RECOGNITION_FEATURES;
        const feature_t* probeVector = &probe_faceprints.data.featuresVector[0];
        const feature_t* anchorVector = nullptr;
        feature_t* galeryAdaptiveVector = nullptr;

        // handle with/without mask vectors properly.
        // choose the correct adaptive vector, and set its flags (based on thresholds configuration).
        switch(thresholds.activeConfig)
        {
            case ThresholdsConfigEnum::ThresoldConfig_pM_gNM:
                // since we are here only is should_update=true, then we 
                // set the adaptive withMask[] vector for the first time (with values of the new faceprints).
                //
                // since its the FIRST TIME - it will probably take 10-20 iterations to converge
                // during LimitAdaptiveVector().
                anchorVector = &updated_faceprints.data.adaptiveDescriptorWithoutMask[0];
                galeryAdaptiveVector = &updated_faceprints.data.adaptiveDescriptorWithMask[0];
                ::memcpy(galeryAdaptiveVector, probeVector, sizeof(probe_faceprints.data.featuresVector));
                // mark the vector as "valid with mask"
                galeryAdaptiveVector[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS] = FaVectorFlagsEnum::VecFlagValidWithMask;
#if(ENABLE_MATCHER_DEBUG_LOGS)
                LOG_DEBUG(LOG_TAG, "----> With-mask adaptation (first-time).");
#endif
                break;

            case ThresholdsConfigEnum::ThresoldConfig_pM_gM:
                anchorVector = &updated_faceprints.data.adaptiveDescriptorWithoutMask[0];
                galeryAdaptiveVector =  &updated_faceprints.data.adaptiveDescriptorWithMask[0];
                //galeryAdaptiveVector[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS] = FaVectorFlagsEnum::VecFlagValidWithMask;
#if(ENABLE_MATCHER_DEBUG_LOGS)
                LOG_DEBUG(LOG_TAG, "----> With-mask adaptation (not first-time).");
#endif
                break;

            case ThresholdsConfigEnum::ThresoldConfig_pNM_gNM:
            default:
                anchorVector = &updated_faceprints.data.enrollmentDescriptor[0];
                galeryAdaptiveVector = &updated_faceprints.data.adaptiveDescriptorWithoutMask[0];
                // galeryAdaptiveVector[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS] = FaVectorFlagsEnum::VecFlagValidWithoutMask;
#if(ENABLE_MATCHER_DEBUG_LOGS)
                LOG_DEBUG(LOG_TAG, "----> Without-mask adaptation.");
#endif
                break;
        }
                
        // blend the current adaptive galery vector with the new vector
        BlendAverageVector(galeryAdaptiveVector, probeVector, vec_length);
        
        // make sure blended adaptive vector is not too far from enrollment vector
        bool update_was_ok = LimitAdaptiveVector(galeryAdaptiveVector, anchorVector,
                                                thresholds, vec_length);

        // disable update flag if something went wrong in the update process.
        result.should_update = result.should_update && update_was_ok;
    }
 
    return result;
}

bool Matcher::LimitAdaptiveVector(feature_t* adaptive_faceprints_vec, const feature_t* anchor_faceprints_vec, 
                                    const Thresholds& thresholds, const uint32_t vec_length)                             
{
    bool success = true;

    if((nullptr == anchor_faceprints_vec) || (nullptr == adaptive_faceprints_vec))
    {
        LOG_ERROR(LOG_TAG, "Null pointer detected : Skipping function.");

        return false;
    }
    // Explain - as long as the adaptive vector is "too far" from the anchor vector, we
    // want to update the adaptive vector with more samples of the anchor vector.
    // hence refreshing the adaptive to be more similar to the anchor vector.
   
    match_calc_t match_score = 0;
    MatchTwoVectors(adaptive_faceprints_vec, anchor_faceprints_vec, &match_score, vec_length);

#if(ENABLE_MATCHER_DEBUG_LOGS)
    LOG_DEBUG(LOG_TAG, "----> match score (adaptive vs. anchor) = %d.", match_score);
#endif

    // adding limit on number of iterations, e.g. if one vector is all zeros we'll get 
    // deadlock here.
    uint32_t cnt_iter = 0;

    uint32_t limit_num_iters = static_cast<uint32_t>(RSID_LIMIT_NUM_ITERS_NM);
    if(thresholds.activeConfig != ThresholdsConfigEnum::ThresoldConfig_pNM_gNM)
    {
        limit_num_iters = static_cast<uint32_t>(RSID_LIMIT_NUM_ITERS_M);
    }

    while ((match_score < thresholds.activeIdenticalThreshold))
    {
#if(ENABLE_MATCHER_DEBUG_LOGS)
        LOG_DEBUG(LOG_TAG, "----> adaptive vector is far from anchor vector. Doing update while() loop : count = %d. score = %d.", 
            cnt_iter, match_score);
#endif

        BlendAverageVector(adaptive_faceprints_vec, anchor_faceprints_vec, vec_length);

        MatchTwoVectors(adaptive_faceprints_vec, anchor_faceprints_vec, &match_score, vec_length);

        cnt_iter++;
        if(cnt_iter > limit_num_iters)
        {
#if(ENABLE_MATCHER_DEBUG_LOGS)
            LOG_DEBUG(LOG_TAG, "----> Update while() loop count reached the limit of %d iterations. Breaking the while() loop with score = %d.", 
                cnt_iter, match_score);
#endif

            success = false;
            break;
        }
    }

    return success;
}

static void ConvertFaceprintsToUserFaceprints(const Faceprints& faceprints, UserFaceprints_t& extended_faceprints)
{
    extended_faceprints.faceprints.data.version = faceprints.data.version;
    extended_faceprints.faceprints.data.featuresType = faceprints.data.featuresType;
    extended_faceprints.faceprints.data.flags = faceprints.data.flags;

    static_assert(sizeof(extended_faceprints.faceprints.data.adaptiveDescriptorWithoutMask) == sizeof(faceprints.data.adaptiveDescriptorWithoutMask), "updated faceprints (with mask) sizes don't match");
    ::memcpy(&extended_faceprints.faceprints.data.adaptiveDescriptorWithoutMask[0], &faceprints.data.adaptiveDescriptorWithoutMask[0], sizeof(faceprints.data.adaptiveDescriptorWithoutMask));

    static_assert(sizeof(extended_faceprints.faceprints.data.adaptiveDescriptorWithMask) == sizeof(faceprints.data.adaptiveDescriptorWithMask), "updated faceprints (without) sizes don't match");
    ::memcpy(&extended_faceprints.faceprints.data.adaptiveDescriptorWithMask[0], &faceprints.data.adaptiveDescriptorWithMask[0], sizeof(faceprints.data.adaptiveDescriptorWithMask));

    static_assert(sizeof(extended_faceprints.faceprints.data.enrollmentDescriptor) == sizeof(faceprints.data.enrollmentDescriptor), "enrollment faceprints sizes don't match");
    ::memcpy(&extended_faceprints.faceprints.data.enrollmentDescriptor[0], &faceprints.data.enrollmentDescriptor[0], sizeof(faceprints.data.enrollmentDescriptor));
}

MatchResultInternal Matcher::MatchFaceprints(const MatchElement& probe_faceprints, const Faceprints& existing_faceprints, Faceprints& updated_faceprints)
{
    // init match result
    MatchResultInternal matchResult;
    matchResult.success = false;
    matchResult.should_update = false;
    matchResult.score = 0;
    matchResult.confidence = 0;

    /*
    // No need to validate probe_faceprints here because its done during MatchFaceprintsToArray() below.
    */

    if (!ValidateFaceprints(existing_faceprints)) 
	{
        LOG_ERROR(LOG_TAG, "existing faceprints vector : failed range validation.");
		return matchResult;	
	}

    // init existing faceprints array
    UserFaceprints_t existing_extended_faceprints;
    ConvertFaceprintsToUserFaceprints(existing_faceprints, existing_extended_faceprints);
    std::vector<UserFaceprints_t> existing_faceprints_array = {existing_extended_faceprints};

    // match using shared code
    ExtendedMatchResult result = MatchFaceprintsToArray(probe_faceprints, existing_faceprints_array, updated_faceprints);

#if(ENABLE_MATCHER_DEBUG_LOGS)
    LOG_DEBUG(LOG_TAG, "Match score: %f, confidence: %f, isSame: %d, shouldUpdate: %d", float(result.maxScore), float(result.confidence), 
                result.isSame, result.should_update);
#endif

    // set results into output struct
    matchResult.score = result.maxScore;
    matchResult.confidence = result.confidence;
    matchResult.success = result.isSame;

    matchResult.should_update = result.should_update;

    return matchResult;
}

Thresholds Matcher::GetDefaultThresholds()
{
    Thresholds thresholds;
    SetToDefaultThresholds(thresholds);
    return thresholds;
}

// NOTE - Here below we have functions with fixed point calculations.
// These are suitable for fixed-point arithmetic (and feature vectors are integer-valued respectively).
//
bool Matcher::ValidateVector(const feature_t* vec, const uint32_t vec_length)
{
    for (uint32_t i = 0; i < vec_length; i++)
    {
        feature_t curr_feature = (feature_t)vec[i];

        if (curr_feature > s_maxFeatureValue || curr_feature < -s_maxFeatureValue)
        {       
            return false;
        }
    }

    return true;
}

void Matcher::BlendAverageVector(feature_t* user_adaptive_faceprints, const feature_t* user_probe_faceprints,
                                        const uint32_t vec_length)
{
    // FUNCTION EXPLAINED :
    //
    // "AV" is the adaptive vector, "NV" is the new vector.
    //
    // we aim to do weighted sum :
    // 		v = round( (30/31)*AV + (1/31)*NV )
    // so we do:
    // 		v = int( (30/31)*AV + (1/31)*NV +/- 1/2)
    // lets multiply all by 2 and then divide by 2 :
    //		v = int ( [((30*2)/31)*AV + (2/31)*NV +/- 1] * 1/2 )
    //      v = int (2*30*AV + 2*NV +/- 31) / (2*31)
    //
    if((nullptr == user_adaptive_faceprints) || (nullptr == user_probe_faceprints))
    {
        LOG_ERROR(LOG_TAG, "Null pointer detected : Skipping function.");
        return; 
    }

    int history_weight = RSID_UPDATE_GALLERY_HISTORY_WEIGHT;
    int round_value = (history_weight + 1);
    for (uint32_t i = 0; i < vec_length ; ++i)
    {
        int32_t v = static_cast<int32_t>(user_adaptive_faceprints[i]);
        v *= 2 * history_weight;
        v += 2 * (int)(user_probe_faceprints[i]);
        v = (v >= 0) ? (v + round_value) : (v - round_value);
        v /= (2 * round_value);

        short feature_value = static_cast<short>(v);
        feature_value = (feature_value > s_maxFeatureValue) ? s_maxFeatureValue : feature_value;
        feature_value = (feature_value < -s_maxFeatureValue) ? -s_maxFeatureValue : feature_value;

        user_adaptive_faceprints[i] = static_cast<short>(v);
    }
}

match_calc_t Matcher::CalculateConfidence(match_calc_t score)
{
    int32_t confidence = 0;
    int32_t min_confidence = 0;
    int32_t m, s, a;

    if (score >= (match_calc_t)RSID_LIN1_SCORE_1)
    {
        m = s_linCurveMultiplier1;
        s = s_linCurveSabtractive1;
        a = s_linCurveAdditive1;
    }
    else if (score >= (match_calc_t)RSID_LIN2_SCORE_1)
    {
        m = s_linCurveMultiplier2;
        s = s_linCurveSabtractive2;
        a = s_linCurveAdditive2;
    }
    else
    {
        // will force confidence = 0.
        m = 0;
        s = 0;
        a = 0;
    }

    // Example on the procedure below : suppose score = 840,  and in our case : m = 7, s = 970, a = 194560.
    // so without rounding we'll get: 
    //      confidence = m*(score-s)+a = 193650 and after shift-back (int)(193650/2048) = 94.
    // but we want a more accurate rounded result so we apply :  
    //      confidence = m*(score-s)+a + 1024 = 194674 and after shift-back (int)(194674/2048) = 95.
    
    // we apply a linear curve from score axis [score1, score2]
    // to confidence axis [confidence1, confidenc2]
    //
    confidence = m * (static_cast<int32_t>(score) - s) + a;
    // assure minimal confidence is 0
    confidence = std::max(confidence, min_confidence);

    // Apply round here so after the shift-back we'll have the rounded (integer) result.
    // since confidence is positive here - we just add a scaled 1/2 before the shift back.
    int32_t round_bit_offset = (0x1 << (s_linCurveHeadroom1 - 1));
    confidence += round_bit_offset;
        
    // shift back
    confidence >>= s_linCurveHeadroom1;
        
    // assure maximal confidence limit
    confidence = std::min(confidence, static_cast <int32_t>(RSID_MAX_POSSIBLE_CONFIDENCE));
 
    return static_cast<match_calc_t>(confidence);
}

short Matcher::GetMsb(const uint32_t ux)
{
    // we find the msb index of a positive integer.
    // 
    // index count starts from 1, so for example: msb of 0x10 is 2 , msb of 0x1011 is 4.
    // exception : msb of 0 returns 0.
    // optimization - we avoid while() loop, and make efficient constant time performance.
    // method - we check the range of x in a binary-search manner.
    // note that we basically compute here msb = floor(log2(x))+1.
    //
    uint32_t x = ux;
    uint32_t shift = 0;
    uint32_t msb = 0; 

    msb = (x > 0xFFFF) << 4; x >>= msb;
    shift = (x > 0xFF) << 3; x >>= shift; msb |= shift;
    shift = (x > 0xF) << 2; x >>= shift; msb |= shift; 
    shift = (x > 0x3) << 1; x >>= shift; msb |= shift;
                                         msb |= (x >> 1);

    // msb of 0 is 0. For any other - add 1 to msb index.
    msb = (ux == 0) ? 0 : (msb + 1);

    return static_cast<short>(msb);
}

void Matcher::MatchTwoVectors(const feature_t* T1, const feature_t* T2, match_calc_t* match_score, const uint32_t vec_length)
{
    // Normalized cross-correlation (ncc) is calculate here with integer arithmetic only.
    // negative cross-correlation will be considered 0 - good for our needs.
    //
    // out = ncc(vec1, vec2)
    //
    // Correct calculation is expected if :
    //
    // (1) all feature vector values are integers in range R = (-1024, +1024).
    // (2) length of vectors is 256.
    //
    // IMPORTANT : If one of the assumptions is violated (longer vectors, or wider range R) - then the calculation may
    // be incorrect due to overflow during bit shifts.
    // We validate these conditions in ValidateFaceprints().
    //
    // The calculated ncc will be an integer in range [0, 4096], with 4096 expected for equal vectors (that satisfy the
    // assumptions above).
    //
    if(nullptr == match_score)
    {
        LOG_ERROR(LOG_TAG, "Null pointer detected : Skipping function.");
        return; 
    }

    //  "Matcher may require carefull adjustments and checks for vectors longer than 256."
    if (vec_length > 256)
    {
        LOG_ERROR(LOG_TAG, "Vector length > 256 : Matcher may require carefull adjustments. Skipping this function, please check!");

        *match_score = 0;
        
        return;
    }

    uint32_t nfeatures = vec_length;
    
    int32_t corr = 0;
    int32_t min_corr = 0;
    uint32_t ucorr = 0;
    uint32_t norm1 = 0;
    uint32_t norm2 = 0;

    for (uint32_t i = 0; i < nfeatures; ++i)
    {
        int32_t t1 = static_cast<int32_t>(T1[i]);
        int32_t t2 = static_cast<int32_t>(T2[i]);

        corr += t1 * t2;
        norm1 += t1 * t1;
        norm2 += t2 * t2;
    }

    // protect division by 0.
    norm1 = (norm1 == 0) ? 1 : norm1;
    norm2 = (norm2 == 0) ? 1 : norm2;

    // negative correlation will be considered as 0 correlation.
    ucorr = static_cast<uint32_t>(std::max(corr, min_corr));

    short norm1_msb = GetMsb(norm1);
    short norm2_msb = GetMsb(norm2);
    short corr_msb = GetMsb(ucorr);
    int32_t min_shift = 0;

    short max_corr_shift = 32 - corr_msb;
    short max_shift1 = 16 - static_cast<short>(std::max(static_cast<int32_t>(corr_msb - norm1_msb), min_shift));
    short max_shift2 = 16 - static_cast<short>(std::max(static_cast<int32_t>(corr_msb - norm2_msb), min_shift));

    short shift1 = static_cast<short>(std::min(static_cast<int32_t>(max_shift1), static_cast<int32_t>(max_corr_shift)));
    short shift2 = static_cast<short>(std::min(static_cast<int32_t>(max_shift2), static_cast<int32_t>(max_corr_shift)));
    short total_shift = shift1 + shift2;
    short shift_back = total_shift - 12;

    uint32_t norm_corr1 = (ucorr << shift1) / norm1;
    uint32_t norm_corr2 = (ucorr << shift2) / norm2;
    uint32_t similarity = norm_corr1 * norm_corr2;

    uint32_t grade = 0;

    if (shift_back >= 0)
    {
        grade = (similarity >> shift_back);
    }
    else
    {
        grade = (similarity << (-shift_back));
    }

    // LOG_DEBUG(LOG_TAG, "ncc result: -----> grade = %u", grade);

    *match_score = static_cast<match_calc_t>(grade);
}

} // namespace RealSenseID
