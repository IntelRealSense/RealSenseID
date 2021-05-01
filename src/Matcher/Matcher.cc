// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "Matcher.h"
#include "Logger.h"
#include "RealSenseID/Faceprints.h"
#include "ExtendedFaceprints.h"
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

static const match_calc_t s_strongThreshold = static_cast<match_calc_t>(RSID_STRONG_THRESHOLD);
static const match_calc_t s_identicalPersonThreshold = static_cast<match_calc_t>(RSID_IDENTICAL_PERSON_THRESHOLD);
static const match_calc_t s_updateThreshold = static_cast<match_calc_t>(RSID_UPDATE_THRESHOLD);

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
    bool versionsMatch = (newFaceprints.version == existingFaceprints.version);
    if (!versionsMatch)
    {
        LOG_ERROR(LOG_TAG, "Faceprints versions don't match");
    }
    return versionsMatch;
}

bool Matcher::GetScores(const Faceprints& new_faceprints,
                        const std::vector<ExtendedFaceprints>& existing_faceprints_array, TagResult& result,
                        match_calc_t threshold)
{
    // initialize.
    result.score = 0;
    result.id = -1;

    if (existing_faceprints_array.size() == 0)
    {
        return false;
    }

    const feature_t* queryFea = (feature_t*)(&(new_faceprints.avgDescriptor[0]));

    match_calc_t maxScore = s_minPossibleScore;
    match_calc_t adaptedScore = s_minPossibleScore;
    int numberOfSubjects = (int)existing_faceprints_array.size();
    int maxSubject = -1;
    uint32_t vec_length = RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER;

    for (int subjectIndex = 0; subjectIndex < numberOfSubjects; subjectIndex++)
    {
        adaptedScore = s_minPossibleScore;
        auto& existing_faceprints = existing_faceprints_array[subjectIndex];

        if (existing_faceprints.faceprints.numberOfDescriptors < 1)
        {
            LOG_ERROR(LOG_TAG, "Invalid number of descriptors in faceprints");
            return false; 
        }

        if (!ValidateFaceprints(existing_faceprints.faceprints))
        {
            LOG_ERROR(LOG_TAG, "Invalid faceprints vector range");
            return false;
        }

        if (!IsSameVersion(new_faceprints, existing_faceprints.faceprints))
        {
            LOG_ERROR(LOG_TAG, "Mismatch in faceprints versions");
            return false;
        }

        MatchTwoVectors((feature_t*)queryFea, (feature_t*)(&existing_faceprints.faceprints.avgDescriptor[0]), 
                        &adaptedScore, vec_length);


        if (adaptedScore > maxScore)
        {
            maxScore = adaptedScore;
            maxSubject = static_cast<int>(subjectIndex);
        }

        if (adaptedScore > threshold)
        {
            break;
        }
    }

    result.score = maxScore;
    result.id = maxSubject;

    return true;
}

void Matcher::FaceMatch(const Faceprints& new_faceprints,
                        const std::vector<ExtendedFaceprints>& existing_faceprints_array, ExtendedMatchResult& result,
                        Thresholds& thresholds)
{
    result.isIdentical = false;
    result.isSame = false;
    result.maxScore = 0;
    result.confidence = 0;
    result.userId = -1;
    result.should_update = false;

    TagResult scoresResult;
    match_calc_t threshold = thresholds.strongThreshold;

    bool isScoreSuccess = GetScores(new_faceprints, existing_faceprints_array, scoresResult, threshold);

    if (!isScoreSuccess)
    {
        LOG_ERROR(LOG_TAG, "Failed during GetScores() - please check.");
        return;
    }

    result.maxScore = scoresResult.score;
    result.isSame = scoresResult.score > threshold;
    result.isIdentical = scoresResult.score > s_identicalPersonThreshold;
    result.userId = scoresResult.id;
    
    result.confidence = CalculateConfidence(scoresResult.score, threshold, result);
}

static void SetToDefaultThresholds(Thresholds& thresholds)
{
    thresholds.strongThreshold = s_strongThreshold;
    thresholds.identicalPersonThreshold = s_identicalPersonThreshold;
    thresholds.updateThreshold = s_updateThreshold;
}

bool Matcher::ValidateFaceprints(const Faceprints& faceprints, bool check_orig)
{
    // TODO - carefull handling in MatchTwoVectors() may be required for vectors longer than 256.
    static_assert((RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER <= 256),
                  "Vector length is higher than 256 - may need careful test and check, due to integer arithmetics and "
                  "overflow risk!");

    static_assert((static_cast<int>(RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER) == NUMBER_OF_RECOGNITION_FACEPRINTS),
                  "Faceprints feature vector length mismatch - please check!");

    uint32_t nfeatures = static_cast<uint32_t>(RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER);
    
    bool is_valid = true;

    if(check_orig)
    {
        is_valid = ValidateVector(&faceprints.origDescriptor[0], nfeatures);
    }
    else
    {
        is_valid = ValidateVector(&faceprints.avgDescriptor[0], nfeatures);
    }
    
    if (!is_valid)
    {
        LOG_ERROR(LOG_TAG, "Vector (faceprint) validation failed!");
    }

    return is_valid;
}

ExtendedMatchResult Matcher::MatchFaceprintsToArray(const Faceprints& new_faceprints,
                                                    const std::vector<ExtendedFaceprints>& existing_faceprints_array,
                                                    Faceprints& updated_faceprints)
{
    Thresholds thresholds;
    SetToDefaultThresholds(thresholds);
    ExtendedMatchResult result;
    
    result.userId = -1;
    result.maxScore = 0;

    if (!ValidateFaceprints(new_faceprints)) 
	{
        LOG_ERROR(LOG_TAG, "Faceprints vector failed range validation.");
		return result;	
	}
    
    if(existing_faceprints_array.size() <= 0)
    {
        LOG_ERROR(LOG_TAG, "Faceprints array size is 0.");
        return result;	
    }

    if (new_faceprints.version != existing_faceprints_array[0].faceprints.version) 
    {
        LOG_ERROR(LOG_TAG, "version mismatch between 2 vectors. Skipping this match()!");
		return result;	
    }

    // Try to match the 2 faceprints. And raise should_update flag respecively.
    FaceMatch(new_faceprints, existing_faceprints_array, result, thresholds);
    result.should_update = (result.maxScore >= s_updateThreshold) && result.isSame;

    bool enable_update = true;

    // if should_update then we create an update vector such that:
    // (1) the current vector is blended into the latest avg vector.
    // (2) then we make sure that the updated avg vector is not too far from the orig. 
    if (result.should_update && enable_update)
    {
        // Init updated_faceprints to the faceprints already exists in the DB
        //  
        size_t user_index = (size_t)result.userId;

        if((user_index < 0) || (user_index >= existing_faceprints_array.size()))
        {
            LOG_ERROR(LOG_TAG, "Invalid user_index : Skipping function.");
            return result;
        }

        updated_faceprints = existing_faceprints_array[user_index].faceprints;
        
        const uint32_t vec_length = RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER;

        // blend the current avg vector with the new vector
        BlendAverageVector(&updated_faceprints.avgDescriptor[0], &new_faceprints.avgDescriptor[0], 
                                vec_length);

        // make sure blended avg vector is not too far from orig vector
        UpdateAverageVector(&updated_faceprints.avgDescriptor[0], &updated_faceprints.origDescriptor[0],
                            vec_length);
    }
    
    return result;
}

ExtendedMatchResult Matcher::MatchFaceprintsToArray(const Faceprints& new_faceprints,
                                                    const std::vector<ExtendedFaceprints>& existing_faceprints_array,
                                                    Faceprints& updated_faceprints, Thresholds thresholds)
{
    ExtendedMatchResult result;

    result.userId = -1;
    result.maxScore = 0;

    if (!ValidateFaceprints(new_faceprints)) 
	{
        LOG_ERROR(LOG_TAG, "Faceprints vector failed range validation.");
		return result;	
	}
    
    if(existing_faceprints_array.size() <= 0)
    {
        LOG_ERROR(LOG_TAG, "Faceprints array size is 0.");
        return result;	
    }

    if (new_faceprints.version != existing_faceprints_array[0].faceprints.version) 
    {
        LOG_ERROR(LOG_TAG, "version mismatch between 2 vectors. Skipping this match()!");
		return result;	
    }

    FaceMatch(new_faceprints, existing_faceprints_array, result, thresholds);
    result.should_update = (result.maxScore >= s_updateThreshold) && result.isSame;
   
    bool enable_update = true;

    // if should_update then we create an update vector such that:
    // (1) the current vector is blended into the latest avg vector.
    // (2) then we make sure that the updated avg vector is not too far from the orig. 
    if (result.should_update && enable_update)
    {      
        // Init updated_faceprints to the faceprints already exists in the DB
        //  
        size_t user_index = (size_t)result.userId;

        if((user_index < 0) || (user_index >= existing_faceprints_array.size()))
        {
            LOG_ERROR(LOG_TAG, "Invalid user_index : Skipping function.");
            return result;
        }

        updated_faceprints = existing_faceprints_array[user_index].faceprints;

        const uint32_t vec_length = RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER;

        // update avg vector with the new vector
        BlendAverageVector(&updated_faceprints.avgDescriptor[0], &new_faceprints.avgDescriptor[0],
                                vec_length);

        // make sure avg vector is not too far from orig vector
        UpdateAverageVector(&updated_faceprints.avgDescriptor[0], &updated_faceprints.origDescriptor[0],
                            vec_length);
    }
 
    return result;
}

bool Matcher::UpdateAverageVector(feature_t* updated_faceprints_vec, const feature_t* orig_faceprints_vec,
                                    const uint32_t vec_length)
{
    if((nullptr == orig_faceprints_vec) || (nullptr == updated_faceprints_vec))
    {
        LOG_ERROR(LOG_TAG, "Null pointer detected : Skipping function.");
        return false;
    }
    // Explain - as long as the avg vector is "too far" from the orig vector, we
    // want to update the avg vector with more samples of the orig vector.
    // hence refreshing the avg to be more similar to the orig vector.
   
    match_calc_t match_score = 0;
    MatchTwoVectors(updated_faceprints_vec, orig_faceprints_vec, &match_score, vec_length);

    // adding limit on number of iterations, e.g. if one vector is all zeros we'll get 
    // deadlock here.
    uint32_t cnt_iter = 0;
    while ((match_score < s_identicalPersonThreshold))
    {
        LOG_DEBUG(LOG_TAG, "----> Avg vector is far from orig vector. Doing update while() loop : count = %d. score = %d.", 
            cnt_iter, match_score);

        BlendAverageVector(updated_faceprints_vec, orig_faceprints_vec, vec_length);

        MatchTwoVectors(updated_faceprints_vec, orig_faceprints_vec, &match_score, vec_length);

        cnt_iter++;
        if(cnt_iter > 10)
        {
            LOG_DEBUG(LOG_TAG, "----> Update while() loop count reached the limit of %d iterations. Breaking the while() loop with score = %d.", 
                cnt_iter, match_score);

            break;
        }
    }

    return true;
}

static void ConvertFaceprintsToExtendedFaceprints(const Faceprints& faceprints, ExtendedFaceprints& extended_faceprints)
{
    extended_faceprints.faceprints.version = faceprints.version;
    extended_faceprints.faceprints.numberOfDescriptors = faceprints.numberOfDescriptors;
    
    static_assert(sizeof(extended_faceprints.faceprints.avgDescriptor) == sizeof(faceprints.avgDescriptor), "faceprints avg sizes don't match");
    ::memcpy(&extended_faceprints.faceprints.avgDescriptor[0], &faceprints.avgDescriptor[0], sizeof(faceprints.avgDescriptor));
    
    static_assert(sizeof(extended_faceprints.faceprints.origDescriptor) == sizeof(faceprints.origDescriptor), "faceprints orig sizes don't match");
    ::memcpy(&extended_faceprints.faceprints.origDescriptor[0], &faceprints.origDescriptor[0], sizeof(faceprints.origDescriptor));
}

MatchResultInternal Matcher::MatchFaceprints(const Faceprints& new_faceprints, const Faceprints& existing_faceprints, Faceprints& updated_faceprints)
{
    // init match result
    MatchResultInternal matchResult;
    matchResult.success = false;
    matchResult.should_update = false;
    matchResult.score = 0;
    matchResult.confidence = 0;

    if (!ValidateFaceprints(new_faceprints)) 
	{
        LOG_ERROR(LOG_TAG, "new faceprints vector : failed range validation.");
		return matchResult;	
	}
    
    if (!ValidateFaceprints(existing_faceprints)) 
	{
        LOG_ERROR(LOG_TAG, "existing faceprints vector : failed range validation.");
		return matchResult;	
	}

    // init existing faceprints array
    ExtendedFaceprints existing_extended_faceprints;
    ConvertFaceprintsToExtendedFaceprints(existing_faceprints, existing_extended_faceprints);
    std::vector<ExtendedFaceprints> existing_faceprints_array = {existing_extended_faceprints};

    // match using shared code
    ExtendedMatchResult result = MatchFaceprintsToArray(new_faceprints, existing_faceprints_array, updated_faceprints);

    LOG_DEBUG(LOG_TAG, "Match score: %f, confidence: %f, isSame: %d, shouldUpdate: %d", float(result.maxScore), float(result.confidence), 
                result.isSame, result.should_update);

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

void Matcher::BlendAverageVector(feature_t* user_average_faceprints, const feature_t* user_new_faceprints,
                                        const uint32_t vec_length)
{
    // FUNCTION EXPLAINED :
    //
    // "avg" is the avreaged vector, "new" is the new vector.
    //
    // we aim to do weighted sum :
    // 		v = round( (30/31)*avg + (1/31)*new )
    // so we do:
    // 		v = int( (30/31)*avg + (1/31)*new +/- 1/2)
    // lets multiply all by 2 and then divide by 2 :
    //		v = int ( [((30*2)/31)*avg + (2/31)*new +/- 1] * 1/2 )
    //      v = int (2*30*avg + 2*new +/- 31) / (2*31)
    //
    if((nullptr == user_average_faceprints) || (nullptr == user_new_faceprints))
    {
        LOG_ERROR(LOG_TAG, "Null pointer detected : Skipping function.");
        return; 
    }

    int history_weight = RSID_UPDATE_GALLERY_HISTORY_WEIGHT;
    int round_value = (history_weight + 1);
    for (uint32_t i = 0; i < vec_length ; ++i)
    {
        int32_t v = static_cast<int32_t>(user_average_faceprints[i]);
        v *= 2 * history_weight;
        v += 2 * (int)(user_new_faceprints[i]);
        v = (v >= 0) ? (v + round_value) : (v - round_value);
        v /= (2 * round_value);

        short feature_value = static_cast<short>(v);
        feature_value = (feature_value > s_maxFeatureValue) ? s_maxFeatureValue : feature_value;
        feature_value = (feature_value < -s_maxFeatureValue) ? -s_maxFeatureValue : feature_value;

        user_average_faceprints[i] = static_cast<short>(v);
    }
}

match_calc_t Matcher::CalculateConfidence(match_calc_t score, match_calc_t threshold, ExtendedMatchResult& result)
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
    // we find the msb index of a positive integer (index starts from 1).
    // for example: msb of 0x10 is 2 , msb of 0x1011 is 4.
    // exception : msb of 0 returns 0.
    // optimize - avoid while() loop, and make efficient constant time performance.
    // method - we check the range of x in a binary-search manner.

    uint32_t x = ux;
    uint32_t shift = 0;
    uint32_t msb = 0; // will be set with the result = floor(log2(x))+1.

    msb = (x > 0xFFFF) << 4; x >>= msb;
    shift = (x > 0xFF) << 3; x >>= shift; msb |= shift;
    shift = (x > 0xF) << 2; x >>= shift; msb |= shift; 
    shift = (x > 0x3) << 1; x >>= shift; msb |= shift;
                                         msb |= (x >> 1);

    // msb of 0 is 0. For any other - add 1 to msb index.
    msb = (ux == 0) ? 0 : (msb + 1);

    return static_cast<short>(msb);
}

void Matcher::MatchTwoVectors(const feature_t* T1, const feature_t* T2, match_calc_t* retprob, const uint32_t vec_length)
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
    if(nullptr == retprob)
    {
        LOG_ERROR(LOG_TAG, "Null pointer detected : Skipping function.");
        return; 
    }

    //  "Matcher may require carefull adjustments and checks for vectors longer than 256."
    if (vec_length > 256)
    {
        LOG_ERROR(LOG_TAG, "Vector length > 256 : Matcher may require carefull adjustments. Skipping this function, please check!");

        *retprob = 0;
        
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

    // std::cout << "ncc result: -----> grade =  " << grade << std::endl;

    // LOG_DEBUG(LOG_TAG, "ncc result: -----> grade = %u", grade);

    *retprob = static_cast<match_calc_t>(grade);
}

} // namespace RealSenseID
