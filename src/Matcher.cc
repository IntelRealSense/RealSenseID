// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "Matcher.h"
#include "Logger.h"
#include "RealSenseID/Faceprints.h"
#include <assert.h>
#include <cstring>

static const int s_minExperienceForStrongThreshold = 0;
static const int s_maxExperienceForStrongThreshold = 25;
static const int s_lowConfidence = 70;
static const int s_minConfidence = 90;
static const int s_highConfidence = 100;
static const int s_maxUserIdSize = 16; // including null character
static const float s_strongThreshold = 630;
static const float s_samePersonThreshold = 620;
static const float s_weakThreshold = 600;
static const float s_identicalPersonThreshold = 730;

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
        float faceprints[RSID_NUMBER_OF_RECOGNITION_FACEPRINTS];
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

    static bool IsFaceprintsVersionValid(const Faceprints& faceprints)
    {
        switch (faceprints.version)
        {
        case 1: {
            return true;
        }
        break;
        default:
            return false;
        }        
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
            original.avgDescriptor[i] *= numberOfDescriptors;
            original.avgDescriptor[i] += latest.faceprints[i];
            original.avgDescriptor[i] /= (numberOfDescriptors + 1);
        }       
    }

    
    static float CalculateConfidence(float score, float thredshold, RecognitionMatchResult& result)
    {   
        float confidence = 0;

        if (score > thredshold)
            confidence = (s_highConfidence - s_minConfidence) / (s_identicalPersonThreshold - s_samePersonThreshold) *
                             (score - s_samePersonThreshold) + s_minConfidence;

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

    static int MatchFaceprintsToFaceprints(float* __restrict T1, float* __restrict T2, float* retprob)
    {
        assert(retprob != NULL);

        float sum = 0;
        int nfaceprints = RSID_NUMBER_OF_RECOGNITION_FACEPRINTS;
        for (int i = 0; i < nfaceprints; ++i)
        {
            float diff = T1[i] - T2[i];
            float mul = diff * diff;
            sum += mul;
        }
        float similarity = 3 - sum;
        sum = 300 * similarity;

        *retprob = sum;
        return 1;
    }

    static void GetScores(const RawFaceprints& new_face_faceprints, const Faceprints& existing_faceprints,
                          TagResult& result,
                          float threshold)
    {
        const float* queryFea = &(new_face_faceprints.faceprints[0]);

        result.idx = -1;
        float NEG_MAX = -1e+20f;
        float maxScore = NEG_MAX;
        float adaptedScore = NEG_MAX;

        if (existing_faceprints.numberOfDescriptors < 1)
		{
            return;
		}

        MatchFaceprintsToFaceprints((float*)queryFea, (float*)(existing_faceprints.avgDescriptor), &adaptedScore);

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

        result.maxScore     = scoresResult.score;        
        result.isSame       = scoresResult.score > threshold;
        result.isIdentical  = scoresResult.score > s_identicalPersonThreshold;
        result.isSimilar    = (!result.isSame) && (scoresResult.score > s_weakThreshold);        
        result.confidence   = CalculateConfidence(scoresResult.score, threshold, result);       
    }

    MatchResult Matcher::MatchFaceprints(const Faceprints& new_faceprints, const Faceprints& existing_faceprints,
                                         Faceprints& updatedFaceprints)    
    {
        MatchResult matchResult;
        matchResult.success = false;
        matchResult.should_update = false;        
        
        RawFaceprints new_raw_faceprints;
        if (!IsFaceprintsVersionValid(new_faceprints) || !IsFaceprintsVersionValid(existing_faceprints))
            return matchResult;

        InitRawFaceprints(new_raw_faceprints, new_faceprints);        

        RecognitionMatchResult result;
        FaceMatch(new_raw_faceprints, existing_faceprints, result);
        Faceprints bestMatchedFaceprints; // TODO: make FaceMatch write to chosenFaceprints

        LOG_DEBUG(LOG_TAG, "Match score: %f", result.maxScore);

        if (result.isSame)
        {
            matchResult.success = true; // TODO: verify exact condition for 'success'

            // TODO: Handle updateFaceprints in a separate API, verify the condition for Updating , it's not only
            // isSame
            //if (!result.isIdentical)
            //{                
                // UpdateFaceprints(bestMatchedFaceprints, faceFaceprints);
                // memcpy_s(&updatedFaceprints, sizeof(updatedFaceprints), &bestMatchedFaceprints,
                // sizeof(bestMatchedFaceprints));
                //matchResult.should_update = true;
            //}
        }

        return matchResult;  
    }
} // namespace RealSenseID
