#pragma once

// included for RECOGNITION_OUTPUT_SIZE
// #include "../../../Algorithms/Shared/InferenceEngine/include/models_size.h"

// These are device/host shared definitions.
//
// Two flags here to enable/disable 2 new features.
//
// RSID_FLAG_USE_INTEGER_VALUED_FEATURE_VECTORS - to enable using integer-valued feature vectors (instead of floats used so far). 
// this feature is now enabled.
//
// the norm will be saved in the last element of the vector. This feature is NOT YET enabled (to be continued later on).
//
#define RSID_FLAG_USE_INTEGER_VALUED_FEATURE_VECTORS    (1)

//
//
#define RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER	(256)
// #define RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER		(RECOGNITION_OUTPUT_SIZE) // take the value defined in models_size.h.

// TODO yossidan - we'll need to change here when we add norm element to vectors.
#define RSID_FEATURES_VECTOR_ALLOC_SIZE			(RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER)

#if (RSID_FLAG_USE_INTEGER_VALUED_FEATURE_VECTORS)

#define RSID_MAX_FEATURE_VALUE					(1023)

#define RSID_MIN_POSSIBLE_SCORE					(0)
#define RSID_MAX_POSSIBLE_SCORE                 (4096)

#define RSID_MAX_POSSIBLE_CONFIDENCE            (100)

// Matching thresholds in the "integers" world.
#define RSID_IDENTICAL_PERSON_THRESHOLD			(2000) 
#define RSID_STRONG_THRESHOLD					(970)  
#define RSID_WEAK_THRESHOLD						(800)
#define RSID_UPDATE_THRESHOLD					(1100) 

// we apply a linear curve from score axis [score1, score2]
// to confidence axis [confidence1, confidenc2].
// note : score2 > score1, and confidence2 > confidenc1.
// for multiplier m, additive a, sabtractive s :
// y = m * (x - s) + a
//
// confidence range is [0,100]
// score range is [0,4096]

// piecewise line curve 1:
#define RSID_LIN1_SCORE_1                (static_cast<int>(RSID_STRONG_THRESHOLD))
#define RSID_LIN1_SCORE_2                (static_cast<int>(RSID_IDENTICAL_PERSON_THRESHOLD))
#define RSID_LIN1_CONFIDENCE_1           (static_cast<int>(95))
#define RSID_LIN1_CONFIDENCE_2           (static_cast<int>(99))
#define RSID_LIN1_CURVE_HR               (static_cast<int>(11))

#define RSID_LIN1_CURVE_MULTIPLIER       (static_cast<int>((((RSID_LIN1_CONFIDENCE_2 - RSID_LIN1_CONFIDENCE_1) << RSID_LIN1_CURVE_HR) / (RSID_LIN1_SCORE_2 - RSID_LIN1_SCORE_1))))
#define RSID_LIN1_CURVE_ADDITIVE         (static_cast<int>((RSID_LIN1_CONFIDENCE_1 << RSID_LIN1_CURVE_HR)))
#define RSID_LIN1_CURVE_SABTRACTIVE      (static_cast<int>(RSID_LIN1_SCORE_1))

// piecewise line curve 2:
#define RSID_LIN2_SCORE_1				(static_cast<int>(RSID_WEAK_THRESHOLD))
#define RSID_LIN2_SCORE_2               (static_cast<int>(RSID_STRONG_THRESHOLD))
#define RSID_LIN2_CONFIDENCE_1          (static_cast<int>(60))
#define RSID_LIN2_CONFIDENCE_2          (static_cast<int>(95))
#define RSID_LIN2_CURVE_HR				(static_cast<int>(11))
#define RSID_LIN2_CURVE_MULTIPLIER      (static_cast<int>((((RSID_LIN2_CONFIDENCE_2 - RSID_LIN2_CONFIDENCE_1) << RSID_LIN2_CURVE_HR) / (RSID_LIN2_SCORE_2 - RSID_LIN2_SCORE_1))))
#define RSID_LIN2_CURVE_ADDITIVE		(static_cast<int>((RSID_LIN2_CONFIDENCE_1 << RSID_LIN2_CURVE_HR)))
#define RSID_LIN2_CURVE_SABTRACTIVE		(static_cast<int>(RSID_LIN2_SCORE_1))


#else

#define RSID_MAX_POSSIBLE_CONFIDENCE    (100.0f)

// Matching thresholds in the "floats" world.
#define RSID_IDENTICAL_PERSON_THRESHOLD (static_cast<float>(720))
#define RSID_STRONG_THRESHOLD           (static_cast<float>(605))
#define RSID_WEAK_THRESHOLD             (static_cast<float>(580))
#define RSID_UPDATE_THRESHOLD           (static_cast<float>(650))

// piecewise line curve 1:
#define RSID_LIN1_SCORE_1               (RSID_STRONG_THRESHOLD)
#define RSID_LIN1_SCORE_2               (RSID_IDENTICAL_PERSON_THRESHOLD)
#define RSID_LIN1_CONFIDENCE_1          (static_cast<float>(95.0f))
#define RSID_LIN1_CONFIDENCE_2          (static_cast<float>(99.0f))
#define RSID_LIN1_CURVE_HR              (11)
#define RSID_LIN1_CURVE_MULTIPLIER      (static_cast<float>(((RSID_LIN1_CONFIDENCE_2 - RSID_LIN1_CONFIDENCE_1) / (RSID_LIN1_SCORE_2 - RSID_LIN1_SCORE_1))))
#define RSID_LIN1_CURVE_ADDITIVE		(static_cast<float>((RSID_LIN1_CONFIDENCE_1)))
#define RSID_LIN1_CURVE_SABTRACTIVE		(static_cast<float>(RSID_LIN1_SCORE_1))

// piecewise line curve 2:
#define RSID_LIN2_SCORE_1           (RSID_WEAK_THRESHOLD)
#define RSID_LIN2_SCORE_2           (RSID_STRONG_THRESHOLD)
#define RSID_LIN2_CONFIDENCE_1      (static_cast<float>(60.0f))
#define RSID_LIN2_CONFIDENCE_2      (static_cast<float>(95.0f))
#define RSID_LIN2_CURVE_HR          (0)
#define RSID_LIN2_CURVE_MULTIPLIER  (static_cast<float>(((RSID_LIN2_CONFIDENCE_2 - RSID_LIN2_CONFIDENCE_1) / (RSID_LIN2_SCORE_2 - RSID_LIN2_SCORE_1))))
#define RSID_LIN2_CURVE_ADDITIVE    (static_cast<float>((RSID_LIN2_CONFIDENCE_1)))
#define RSID_LIN2_CURVE_SABTRACTIVE (static_cast<float>(RSID_LIN2_SCORE_1))


#define RSID_MAX_FEATURE_VALUE					(static_cast<float>((1.0f)))
#define RSID_MIN_POSSIBLE_SCORE					(static_cast<float>(-1e+20f))

#endif

