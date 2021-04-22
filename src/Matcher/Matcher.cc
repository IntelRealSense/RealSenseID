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

static bool IsSameVersion(const Faceprints& newFaceprints, const Faceprints existingFaceprints)
{
    bool versionsMatch = (newFaceprints.version == existingFaceprints.version);
    if (!versionsMatch)
        LOG_ERROR(LOG_TAG, "Faceprints versions don't match");
    return versionsMatch;
}

bool Matcher::GetScores(const Faceprints& new_faceprints,
                        const std::vector<ExtendedFaceprints>& existing_faceprints_array, TagResult& result,
                        match_calc_t threshold)
{
    
    if (existing_faceprints_array.size() == 0)
    {
        result.id = -1;
        return false;
    }

    const short* queryFea = &(new_faceprints.avgDescriptor[0]);

    match_calc_t maxScore = s_minPossibleScore;
    match_calc_t adaptedScore = s_minPossibleScore;
    int numberOfSubjects = (int)existing_faceprints_array.size();
    int maxSubject = -1;

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
            LOG_ERROR(LOG_TAG, "Invalid faceprints");
            return false;
        }

        if (!IsSameVersion(new_faceprints, existing_faceprints.faceprints))
        {
            LOG_ERROR(LOG_TAG, "Mismatch in faceprints versions");
            return false;
        }

        MatchFaceprintsToFaceprints((feature_t*)queryFea,
                                    (feature_t*)(existing_faceprints.faceprints.avgDescriptor), &adaptedScore);

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

    result.confidence = CalculateConfidence(scoresResult.score, threshold, result);

    result.userId = scoresResult.id;
}

ExtendedMatchResult Matcher::MatchFaceprintsToArray(const Faceprints& new_faceprints,
                                                    const std::vector<ExtendedFaceprints>& existing_faceprints_array,
                                                    Faceprints& updated_faceprints, Thresholds thresholds)
{
    ExtendedMatchResult result;
    FaceMatch(new_faceprints, existing_faceprints_array, result, thresholds);
    return result;
}

static void SetToDefaultThresholds(Thresholds& thresholds)
{
    thresholds.strongThreshold = s_strongThreshold;
    thresholds.identicalPersonThreshold = s_identicalPersonThreshold;
    thresholds.updateThreshold = s_updateThreshold;
}

ExtendedMatchResult Matcher::MatchFaceprintsToArray(const Faceprints& new_faceprints,
                                                    const std::vector<ExtendedFaceprints>& existing_faceprints_array,
                                                    Faceprints& updated_faceprints)
{
    Thresholds thresholds;
    SetToDefaultThresholds(thresholds);
    ExtendedMatchResult result;
    FaceMatch(new_faceprints, existing_faceprints_array, result, thresholds);
    return result;
}

static void ConvertFaceprintsToExtendedFaceprints(const Faceprints& faceprints, ExtendedFaceprints& extended_faceprints)
{
    extended_faceprints.faceprints.version = faceprints.version;
    extended_faceprints.faceprints.numberOfDescriptors = faceprints.numberOfDescriptors;
    static_assert(sizeof(extended_faceprints.faceprints.avgDescriptor) == sizeof(faceprints.avgDescriptor),
                  "faceprints sizes don't match");
    ::memcpy(&extended_faceprints.faceprints.avgDescriptor, &faceprints.avgDescriptor,
             sizeof(faceprints.avgDescriptor));
}

MatchResultInternal Matcher::MatchFaceprints(const Faceprints& new_faceprints, const Faceprints& existing_faceprints,
                                         Faceprints& updated_faceprints)
{
    // init match result
    MatchResultInternal matchResult;
    matchResult.success = false;
    matchResult.should_update = false;
    matchResult.score = 0;
    matchResult.confidence = 0;

    // validate new faceprints
    if (!ValidateFaceprints(new_faceprints))
        return matchResult;

    // init existing faceprints array
    ExtendedFaceprints existing_extended_faceprints;
    ConvertFaceprintsToExtendedFaceprints(existing_faceprints, existing_extended_faceprints);
    std::vector<ExtendedFaceprints> existing_faceprints_array = {existing_extended_faceprints};

    // match using shared code
    ExtendedMatchResult result = MatchFaceprintsToArray(new_faceprints, existing_faceprints_array, updated_faceprints);

    LOG_DEBUG(LOG_TAG, "Match score: %f, confidence: %f, isSame: %d", float(result.maxScore), float(result.confidence), result.isSame);

    // set results into output struct
    matchResult.score = result.maxScore;
    matchResult.confidence = result.confidence;
    matchResult.success = result.isSame;

    // handle positive match result for host flow
    /*
    if (result.isSame)
    {
        // auto& existing_faceprints = existing_faceprints_array[0];
        matchResult.success = true;

        // disabled until new updating method is integrated
        // if (!result.isIdentical)
        //{
        //    static_assert(sizeof(updated_faceprints) == sizeof(updated_faceprints), "faceprints sizes don't match");
        //    ::memcpy(&updated_faceprints, &existing_faceprints, sizeof(updated_faceprints));
        //    UpdateFaceprints(updated_faceprints, new_faceprints);
        //    if(ValidateFaceprints(updated_faceprints))
        //        matchResult.should_update = true;
        //}
    }
    */

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
bool Matcher::ValidateFaceprints(const Faceprints& faceprints)
{
    uint32_t nfeatures = static_cast<uint32_t>(RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER);
     
    // TODO - carefull handling in MatchFaceprintsToFaceprints() may be required for vectors longer than 256.
    static_assert((RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER <= 256), "Vector length is higher than 256 - may need careful test and check, due to integer arithmetics and "
                  "overflow risk!");

    static_assert((static_cast<int>(RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER) == NUMBER_OF_RECOGNITION_FACEPRINTS), "Faceprints feature vector length mismatch - please check!");

    for (uint32_t i = 0; i < nfeatures; i++)
    {
        int32_t curr_feature = (int32_t)faceprints.avgDescriptor[i];

        if (curr_feature > s_maxFeatureValue || curr_feature < -s_maxFeatureValue)
        {
            LOG_ERROR(LOG_TAG, "faceprint #%d value- %d is invalid",i, curr_feature);
            return false;
        }
    }

    return true;
}
/*
static void UpdateFaceprints(Faceprints& original, const Faceprints& latest)
{
    // TODO - this function needs re-thinking in terms of:
    // (1) correctness
    // (2) integer arithmetic
    // NOTE - currently implementation here is translated from the original floating
    // point function (see below), and it seems incorrect (?!) to use the same numberOfDescriptors
    // in the loop.
    //  
    int32_t numberOfDescriptors = original.numberOfDescriptors++;
    int32_t nbitsHeadroom = 11;
    int32_t round_bit_offset = (0x1 << (nbitsHeadroom - 1));

    for (int i = 0; i < RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER; ++i)
    {
        int32_t v = static_cast <int32_t>(original.avgDescriptor[i]);
        v *= numberOfDescriptors;
        v += static_cast <int32_t>(latest.avgDescriptor[i]);
        // v = (v << nbitsHeadroom) / (numberOfDescriptors + 1);
        v = (v >= 0) ? ((v << nbitsHeadroom) / (numberOfDescriptors + 1)) : -(((-v) << nbitsHeadroom) / (numberOfDescriptors + 1));

        // Apply round here so after the shift-back we'll have the rounded (integer) result.
        v = (v >= 0) ? (v + round_bit_offset) : (v - round_bit_offset);

        // shift back
        v = (v >= 0) ? (v >> nbitsHeadroom) : -((-v) >> nbitsHeadroom);

        original.avgDescriptor[i] = static_cast <short>(v);
    }
}
*/
match_calc_t Matcher::CalculateConfidence(match_calc_t score, match_calc_t threshold,
                                                    ExtendedMatchResult& result)
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

void Matcher::MatchFaceprintsToFaceprints(feature_t* T1, feature_t* T2, match_calc_t* retprob)
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
    assert(retprob != nullptr);
    
    static_assert((RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER <= 256), "Matcher may require carefull adjustments and checks for vectors longer than 256.");

    uint32_t nfeatures = static_cast<uint32_t>(RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER);

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
