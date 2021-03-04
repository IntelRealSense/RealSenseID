// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "Matcher.h"
#include "Logger.h"
#include "RealSenseID/Faceprints.h"
#include <assert.h>
#include <cstring>
#include <stdexcept>

static const int s_minExperienceForStrongThreshold = 0;
static const int s_maxExperienceForStrongThreshold = 25;
static const int s_lowConfidence = 70;
static const int s_minConfidence = 90;
static const int s_highConfidence = 100;
static const int s_maxUserIdSize = 16; // including null character
static const float s_strongThreshold = 605;
static const float s_samePersonThreshold = 600;
static const float s_weakThreshold = 580;
static const float s_identicalPersonThreshold = 700;

#define F16_EXPONENT_BITS  0x1F
#define F16_EXPONENT_SHIFT 10
#define F16_MANTISSA_BITS  ((1 << F16_EXPONENT_SHIFT) - 1)
#define F16_MANTISSA_SHIFT (23 - F16_EXPONENT_SHIFT)
#define F16_EXPONENT_BIAS  15
typedef short FEATURE_TYPE;
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

struct TagResult
{
    int idx;
    char id[s_maxUserIdSize];
    float score;
    float similarityScore;
};

struct RawFaceprints
{
    short faceprints[RSID_NUMBER_OF_RECOGNITION_FACEPRINTS];
};

struct RecognitionMatchResult
{
    RecognitionMatchResult() : isIdentical(false), isSame(false), isSimilar(false), maxScore(0.f), confidence(0.f)
    {
    }

    bool isIdentical;
    bool isSame;
    bool isSimilar;
    char userId[s_maxUserIdSize];
    float maxScore;
    float confidence;
};

static bool ValidateFaceprints(const Faceprints& faceprints)
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
    LOG_DEBUG(LOG_TAG, "faceprints absolute-average: %f", abs_avg / (float)RSID_NUMBER_OF_RECOGNITION_FACEPRINTS);
    return true;
}

static bool IsSameVersion(const Faceprints& newFaceprints, const Faceprints& existingFaceprints)
{
    bool versionsMatch = (newFaceprints.version == existingFaceprints.version);
    if (!versionsMatch)
        LOG_ERROR(LOG_TAG, "Faceprints versions don't match");
    return versionsMatch;
}

static void InitRawFaceprints(RawFaceprints& rawFaceprints, const Faceprints& faceprints)
{
    switch (faceprints.version)
    {
    case 1: {
        ::memcpy((void*)&rawFaceprints.faceprints, faceprints.avgDescriptor, sizeof(rawFaceprints.faceprints));
    }
    break;
    default:
        LOG_ERROR(LOG_TAG, "Unknown faceprints version: %d", faceprints.version);
    }
}
static void UpdateFaceprints(Faceprints& original, RawFaceprints& latest)
{
    int numberOfDescriptors = original.numberOfDescriptors++;
    for (int i = 0; i < RSID_NUMBER_OF_RECOGNITION_FACEPRINTS; ++i)
    {
		float v = nn_fp16_to_fp32(original.avgDescriptor[i]);
        v *= numberOfDescriptors;
        v += nn_fp16_to_fp32(latest.faceprints[i]);
        v /= (numberOfDescriptors + 1);
		original.avgDescriptor[i] = nn_fp32_to_fp16(v);
		
    }
}


static float CalculateConfidence(float score, float thredshold, RecognitionMatchResult& result)
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

static void MatchFaceprintsToFaceprints(short* T1, short* T2, float* retprob)
{
    assert(retprob != NULL);

    float sum = 0;
    int nfaceprints = RSID_NUMBER_OF_RECOGNITION_FACEPRINTS;
    for (int i = 0; i < nfaceprints; ++i)
    {
        float diff = nn_fp16_to_fp32(T1[i]) - nn_fp16_to_fp32(T2[i]);
        float mul = diff * diff;
        sum += mul;
    }
    float similarity = 3 - sum;
    sum = 300 * similarity;

    *retprob = sum;    
}

static void GetScores(const RawFaceprints& new_face_faceprints, const Faceprints& existing_faceprints,
                      TagResult& result, float threshold)
{
    const short* queryFea = &(new_face_faceprints.faceprints[0]);

    result.idx = -1;
    float NEG_MAX = -1e+20f;
    float maxScore = NEG_MAX;
    float adaptedScore = NEG_MAX;

    if (existing_faceprints.numberOfDescriptors < 1)
    {
        return;
    }

    MatchFaceprintsToFaceprints((short*)queryFea, (short*)(existing_faceprints.avgDescriptor), &adaptedScore);

    if (adaptedScore > maxScore)
    {
        maxScore = adaptedScore;
    }

    result.score = maxScore;
}

static void FaceMatch(RawFaceprints& new_raw_faceprints, const Faceprints& existing_faceprints,
                      RecognitionMatchResult& result)
{
    result.isIdentical = false;
    result.isSame = false;
    result.isSimilar = false;

    result.maxScore = 0;
    result.confidence = 0;

    TagResult scoresResult;
    float threshold = s_strongThreshold;

    GetScores(new_raw_faceprints, existing_faceprints, scoresResult, threshold);

    result.maxScore = scoresResult.score;
    result.isSame = scoresResult.score > threshold;
    result.isIdentical = scoresResult.score > s_identicalPersonThreshold;
    result.isSimilar = (!result.isSame) && (scoresResult.score > s_weakThreshold);
    result.confidence = CalculateConfidence(scoresResult.score, threshold, result);
}

MatchResult Matcher::MatchFaceprints(const Faceprints& new_faceprints, const Faceprints& existing_faceprints,
                                     Faceprints& updated_faceprints)
{
    MatchResult matchResult;
    matchResult.success = false;
    matchResult.should_update = false;

    if (!ValidateFaceprints(new_faceprints) || !ValidateFaceprints(existing_faceprints))
        return matchResult;    

    RawFaceprints new_raw_faceprints;
    if (!IsSameVersion(new_faceprints, existing_faceprints))
        return matchResult;

    InitRawFaceprints(new_raw_faceprints, new_faceprints);

    RecognitionMatchResult result;
    FaceMatch(new_raw_faceprints, existing_faceprints, result);

    LOG_DEBUG(LOG_TAG, "Match score: %f, isSame: %d", result.maxScore, static_cast<int>(result.isSame));

    if (result.isSame)
    {
        matchResult.success = true; // TODO: verify exact condition for 'success' and 'should_update'
        if (!result.isIdentical)
        {
            static_assert(sizeof(updated_faceprints) == sizeof(updated_faceprints), "faceprints sizes don't match");
            ::memcpy(&updated_faceprints, &existing_faceprints, sizeof(updated_faceprints));
            UpdateFaceprints(updated_faceprints, new_raw_faceprints);            
            if(ValidateFaceprints(updated_faceprints))
                matchResult.should_update = true;
        }
    }

    return matchResult;
}
} // namespace RealSenseID
