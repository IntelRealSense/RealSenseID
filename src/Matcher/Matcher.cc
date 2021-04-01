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

static const int s_minExperienceForStrongThreshold = 0;
static const int s_maxExperienceForStrongThreshold = 25;
static const int s_lowConfidence = 70;
static const int s_minConfidence = 90;
static const int s_highConfidence = 100;
static const int s_maxUserIdSize = 31; // including null character
static const float s_strongThreshold = 605;
static const float s_samePersonThreshold = 600;
static const float s_weakThreshold = 580;
static const float s_updateThreshold = 650;
static const float s_identicalPersonThreshold = 720;

#define F16_EXPONENT_BITS  0x1F
#define F16_EXPONENT_SHIFT 10
#define F16_MANTISSA_BITS  ((1 << F16_EXPONENT_SHIFT) - 1)
#define F16_MANTISSA_SHIFT (23 - F16_EXPONENT_SHIFT)
#define F16_EXPONENT_BIAS  15
typedef unsigned short UINT16;
typedef short SINT16;
typedef unsigned int UINT32;
typedef int SINT32;

static SINT16 nn_fp32_to_fp16(float val)
{
	UINT32 f32 = (*(UINT32*)&val);
	SINT16 f16 = 0;
	/* Decode IEEE 754 little-endian 32-bit floating-point value */
	SINT32 sign = (f32 >> 16) & 0x8000;
	/* Map exponent to the range [-127,128] */
	SINT32 exponent = ((f32 >> 23) & 0xff) - 127;
	SINT32 mantissa = f32 & 0x007fffff;
	if (exponent == 128)
	{ /* Infinity or NaN */
		if (mantissa)
		{
			/* Flush NaN to 0. */
			f16 = (SINT16)sign;
		}
		else
		{
			/* Clamp to HALF_MAX/HALF_MIN. */
			f16 = (SINT16)(sign | ((F16_EXPONENT_BITS - 1) << F16_EXPONENT_SHIFT) | F16_MANTISSA_BITS);
		}
	}
	else if (exponent > 15)
	{ /* Overflow - clamp to HALF_MAX/HALF_MIN. */
		f16 = (SINT16)(sign | ((F16_EXPONENT_BITS - 1) << F16_EXPONENT_SHIFT) | F16_MANTISSA_BITS);
	}
	else if (exponent > -15)
	{ /* Representable value */
		/* RTNE */
		SINT32 roundingBit = (mantissa >> (F16_MANTISSA_SHIFT - 1)) & 0x1;
		SINT32 stickyBits  = mantissa & 0xFFF;
		exponent += F16_EXPONENT_BIAS;
		mantissa >>= F16_MANTISSA_SHIFT;
		if (roundingBit)
		{
			if (stickyBits || (mantissa & 0x1))
			{
				mantissa++;
				if (mantissa > F16_MANTISSA_BITS)
				{
					exponent++;
					if (exponent > 30)
					{
						/* Clamp to HALF_MAX/HALF_MIN. */
						exponent--;
						mantissa--;
					}
					else
					{
						mantissa &= F16_MANTISSA_BITS;
					}
				}
			}
		}
		f16 = (SINT16)(sign | exponent << F16_EXPONENT_SHIFT | mantissa);
	}
	else
	{
		f16 = (SINT16)sign;
	}
	return f16;
}

static float nn_fp16_to_fp32(UINT16 val)
{
	float   ret;
	UINT32* pf  = (UINT32*)&ret;
	int     exp = (val & 0x7c00) >> 10;
	exp         = exp - 15 + 127;
	*pf         = ((exp & 0xff) << 23) + ((val & 0x3ff) << 13);
	*pf |= ((val & 0x8000) << 16);

	return ret;
}

namespace RealSenseID
{
static const char* LOG_TAG = "Matcher";

bool Matcher::ValidateFaceprints(const Faceprints& faceprints)
{
    float abs_avg = 0.0;
    for (int i = 0; i < RSID_NUMBER_OF_RECOGNITION_FACEPRINTS; i++)
    {
        float curr_feature = nn_fp16_to_fp32(faceprints.avgDescriptor[i]);
        if (curr_feature > 1 || curr_feature < -1)
        {
            LOG_ERROR(LOG_TAG, "faceprint value is invalid");
            return false;
        }            
        abs_avg += std::abs(curr_feature);
    }    
    return true;
}

static bool IsSameVersion(const Faceprints& newFaceprints, const Faceprints& existingFaceprints)
{
    bool versionsMatch = (newFaceprints.version == existingFaceprints.version);
    if (!versionsMatch)
        LOG_ERROR(LOG_TAG, "Faceprints versions don't match");
    return versionsMatch;
}


static void UpdateFaceprints(Faceprints& original, const Faceprints& latest)
{
    int numberOfDescriptors = original.numberOfDescriptors++;
    for (int i = 0; i < RSID_NUMBER_OF_RECOGNITION_FACEPRINTS; ++i)
    {
		float v = nn_fp16_to_fp32(original.avgDescriptor[i]);
        v *= numberOfDescriptors;
        v += nn_fp16_to_fp32(latest.avgDescriptor[i]);
        v /= (numberOfDescriptors + 1);
		original.avgDescriptor[i] = nn_fp32_to_fp16(v);		
    }
}


static float CalculateConfidence(float score, float thredshold, ExtendedMatchResult& result)
{
    float confidence = 0;

    if (score > thredshold)
        confidence = (s_highConfidence - s_minConfidence) / (s_identicalPersonThreshold - s_samePersonThreshold) *
                         (score - s_samePersonThreshold) +
                     s_minConfidence;

    if (score > s_identicalPersonThreshold)
        confidence = s_highConfidence;

    if (result.isSame)
        return confidence;

    if (score > s_weakThreshold)
    {
        confidence = s_lowConfidence;
        return confidence;
    }

    return confidence;
}

void Matcher::MatchFaceprintsToFaceprints(FEATURE_TYPE* T1, FEATURE_TYPE* T2, float* retprob)
{
    assert(retprob != NULL);
    float sum = 0;    

	if (sizeof(FEATURE_TYPE) == sizeof(short))
	{
		for (int i = 0; i < RSID_NUMBER_OF_RECOGNITION_FACEPRINTS; ++i)
		{

			float diff = nn_fp16_to_fp32(T1[i]) - nn_fp16_to_fp32(T2[i]);
			float mul = diff * diff;
			sum += mul;
		}
	}
	else   
        LOG_ERROR(LOG_TAG, "Invalid faceprints type");
	

    float similarity = 3 - sum;
    sum = 300 * similarity;
    *retprob = sum;    
}

void Matcher::GetScores(const Faceprints& new_faceprints,
                        const std::vector<ExtendedFaceprints>& existing_faceprints_array,
                        TagResult& result, float threshold)
{
    if (existing_faceprints_array.size() == 0)
	{
		result.id = -1;
		return;
	}

    const short* queryFea = &(new_faceprints.avgDescriptor[0]);
    
    float NEG_MAX = -1e+20f;
    float maxScore = NEG_MAX;
    int maxSubject = -1;
    float adaptedScore = NEG_MAX;    
    int numberOfSubjects = (int) existing_faceprints_array.size();
    
    for (int subjectIndex=0; subjectIndex<numberOfSubjects; subjectIndex++)
    {
        adaptedScore = NEG_MAX;
        auto& existing_faceprints = existing_faceprints_array[subjectIndex];

        if (existing_faceprints.faceprints.numberOfDescriptors < 1)
        {
            LOG_ERROR(LOG_TAG, "Invalid number of descriptors in faceprints");            
            return; // same logic as in original matcher
        }

        if (!ValidateFaceprints(existing_faceprints.faceprints))
        {
            LOG_ERROR(LOG_TAG, "Invalid faceprints");            
            return;
        }

        if (!IsSameVersion(new_faceprints, existing_faceprints.faceprints))
        {
            LOG_ERROR(LOG_TAG, "Mismatch in faceprints versions");            
            return;
        }

        MatchFaceprintsToFaceprints((FEATURE_TYPE*)queryFea, (FEATURE_TYPE*)(existing_faceprints.faceprints.avgDescriptor), &adaptedScore);

        if (adaptedScore > maxScore)
		{
			maxScore = adaptedScore;
			maxSubject = static_cast<int>(subjectIndex);
		}
		
		if (adaptedScore > threshold)
            break;
    }
    
    result.score = maxScore;
    result.id = maxSubject;
}

void Matcher::FaceMatch(const Faceprints& new_faceprints,
                        const std::vector<ExtendedFaceprints>& existing_faceprints_array,
                        ExtendedMatchResult& result, Thresholds& thresholds)
{
    result.isIdentical = false;
    result.isSame = false;
    result.isSimilar = false;

    result.maxScore = 0;
    result.confidence = 0;
    result.userId = -1;

    TagResult scoresResult;
    float threshold = thresholds.strongThreshold;

    GetScores(new_faceprints, existing_faceprints_array, scoresResult, threshold);    

    result.maxScore = scoresResult.score;
    result.isSame = scoresResult.score > threshold;
    result.isIdentical = scoresResult.score > s_identicalPersonThreshold;
    result.isSimilar = (!result.isSame) && (scoresResult.score > s_weakThreshold);
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
    thresholds.samePersonThreshold = s_samePersonThreshold;
    thresholds.weakThreshold = s_weakThreshold;
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

    // validate new faceprints
    if (!ValidateFaceprints(new_faceprints))
        return matchResult;

    // init existing faceprints array
    ExtendedFaceprints existing_extended_faceprints;
    ConvertFaceprintsToExtendedFaceprints(existing_faceprints, existing_extended_faceprints);
    std::vector<ExtendedFaceprints> existing_faceprints_array = {existing_extended_faceprints};
   
    // match using shared code
    ExtendedMatchResult result = MatchFaceprintsToArray(new_faceprints, existing_faceprints_array, updated_faceprints);    
    LOG_DEBUG(LOG_TAG, "Match score: %f", result.maxScore);

    // handle positive match result for host flow
    if (result.isSame)
    {        
        //auto& existing_faceprints = existing_faceprints_array[0];
        matchResult.success = true;

        // disabled until new updating method is integrated
        //if (!result.isIdentical)
        //{
        //    static_assert(sizeof(updated_faceprints) == sizeof(updated_faceprints), "faceprints sizes don't match");
        //    ::memcpy(&updated_faceprints, &existing_faceprints, sizeof(updated_faceprints));
        //    UpdateFaceprints(updated_faceprints, new_faceprints);            
        //    if(ValidateFaceprints(updated_faceprints))
        //        matchResult.should_update = true;
        //}
    }

    return matchResult;
}

Thresholds Matcher::GetDefaultThresholds()
{
    Thresholds thresholds;
    SetToDefaultThresholds(thresholds);    
    return thresholds;
}

} // namespace RealSenseID
