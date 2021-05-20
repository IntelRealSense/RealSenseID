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

// 3 extra elements (1 for hasMask , 1 for norm + 1 spare).
#define RSID_FEATURES_VECTOR_ALLOC_SIZE                 (RSID_NUMBER_OF_RECOGNITION_FACEPRINTS_MATCHER+3)
#define RSID_INDEX_IN_FEATURS_VECTOR_HAS_MASK           (256)

#if (RSID_FLAG_USE_INTEGER_VALUED_FEATURE_VECTORS)

#define RSID_MAX_FEATURE_VALUE					(1023)

#define RSID_MIN_POSSIBLE_SCORE					(0)
#define RSID_MAX_POSSIBLE_SCORE                 (4096)

#define RSID_MAX_POSSIBLE_CONFIDENCE            (100)

// Matching thresholds in the "integers" world.
// M - for vectors with mask 
// NM - for vectors with no-mask 
#define RSID_IDENTICAL_THRESHOLD_NM		        (2754) 
#define RSID_IDENTICAL_THRESHOLD_M		        (2754) 

#define RSID_STRONG_THRESHOLD_PNM_GNM			(970)  
#define RSID_STRONG_THRESHOLD_PM_GM			    (970)  
#define RSID_STRONG_THRESHOLD_PM_GNM			(970)  

#define RSID_WEAK_THRESHOLD						(800)
//
#define RSID_UPDATE_THRESHOLD_NM		        (1745) 
#define RSID_UPDATE_THRESHOLD_M		            (1745) 
#define RSID_UPDATE_THRESHOLD_MFIRST		    (1745) 

// we apply a linear curve from score axis [score1, score2]
// to confidence axis [confidence1, confidenc2].
// note : score2 > score1, and confidence2 > confidenc1.
// for multiplier m, additive a, sabtractive s :
// y = m * (x - s) + a
//
// confidence range is [0,100]
// score range is [0,4096]

// piecewise line curve 1:
#define RSID_LIN1_SCORE_1                (static_cast<int>(RSID_STRONG_THRESHOLD_PNM_GNM))
#define RSID_LIN1_SCORE_2                (static_cast<int>(RSID_IDENTICAL_THRESHOLD_NM))
#define RSID_LIN1_CONFIDENCE_1           (static_cast<int>(95))
#define RSID_LIN1_CONFIDENCE_2           (static_cast<int>(99))
#define RSID_LIN1_CURVE_HR               (static_cast<int>(11))

#define RSID_LIN1_CURVE_MULTIPLIER       (static_cast<int>((((RSID_LIN1_CONFIDENCE_2 - RSID_LIN1_CONFIDENCE_1) << RSID_LIN1_CURVE_HR) / (RSID_LIN1_SCORE_2 - RSID_LIN1_SCORE_1))))
#define RSID_LIN1_CURVE_ADDITIVE         (static_cast<int>((RSID_LIN1_CONFIDENCE_1 << RSID_LIN1_CURVE_HR)))
#define RSID_LIN1_CURVE_SABTRACTIVE      (static_cast<int>(RSID_LIN1_SCORE_1))

// piecewise line curve 2:
#define RSID_LIN2_SCORE_1				(static_cast<int>(RSID_WEAK_THRESHOLD))
#define RSID_LIN2_SCORE_2               (static_cast<int>(RSID_STRONG_THRESHOLD_PNM_GNM))
#define RSID_LIN2_CONFIDENCE_1          (static_cast<int>(60))
#define RSID_LIN2_CONFIDENCE_2          (static_cast<int>(95))
#define RSID_LIN2_CURVE_HR				(static_cast<int>(11))
#define RSID_LIN2_CURVE_MULTIPLIER      (static_cast<int>((((RSID_LIN2_CONFIDENCE_2 - RSID_LIN2_CONFIDENCE_1) << RSID_LIN2_CURVE_HR) / (RSID_LIN2_SCORE_2 - RSID_LIN2_SCORE_1))))
#define RSID_LIN2_CURVE_ADDITIVE		(static_cast<int>((RSID_LIN2_CONFIDENCE_1 << RSID_LIN2_CURVE_HR)))
#define RSID_LIN2_CURVE_SABTRACTIVE		(static_cast<int>(RSID_LIN2_SCORE_1))

// Update gallery defines
#define RSID_UPDATE_GALLERY_HISTORY_WEIGHT (static_cast<int>(30))

#else

// only integer-valued vectors supported. Range of vectors is [-1023, + 1023].

#endif

