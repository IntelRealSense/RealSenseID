#pragma once

// Matcher impl definitions.
// note that we work with integer-valued feature vectors (value range [-1023,+1023]).
//

#define RSID_MIN_POSSIBLE_SCORE					(0)
#define RSID_MAX_POSSIBLE_SCORE                 (4096)

#define RSID_MAX_POSSIBLE_CONFIDENCE            (100)

// Integer valued matching thresholds. Used for adaptive-learning with/without mask faceprints. 
// Naming convention:
//		M - for vectors with mask 
//		NM - for vectors with no-mask
//		G - galerry vector
//      P - probe vector
//      * we match between a new faceprint vector (probe user) VS. an existing gallery vector.
//
// so for example - RSID_STRONG_THRESHOLD_PNM_GNM means strong threshold for no-mask probe vector vs. no-mask gallery vector.
//
#define RSID_IDENTICAL_THRESHOLD_GNM_GNM            (2754) 
#define RSID_IDENTICAL_THRESHOLD_GM_GNM             (2754) 

#define RSID_STRONG_THRESHOLD_PNM_GNM               (1224)   
#define RSID_STRONG_THRESHOLD_PM_GM                 (970) // xx – will not be used, until we open with mask adaptive vector.
#define RSID_STRONG_THRESHOLD_PM_GNM                (981) 

#define RSID_UPDATE_THRESHOLD_PNM_GNM               (1745) 
#define RSID_UPDATE_THRESHOLD_PM_GM                 (1745) // xx – will not be used, until we open with mask adaptive vector.
#define RSID_UPDATE_THRESHOLD_PM_GNM_FIRST          (4096) // for now we disable with-mask adaptive opening until mask detector is improved! 

// limit the number of iterations in the while() loop during the update process.
#define RSID_LIMIT_NUM_ITERS_NM						(100)
#define RSID_LIMIT_NUM_ITERS_M						(400)

#define RSID_WEAK_THRESHOLD							(800)

// Update gallery defines
#define RSID_UPDATE_GALLERY_HISTORY_WEIGHT          (30)

// we work with integer-valued feature vectors (value range [-1023,+1023]).
#define RSID_MAX_FEATURE_VALUE                      (1023)

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
#define RSID_LIN1_SCORE_2                (static_cast<int>(RSID_IDENTICAL_THRESHOLD_GNM_GNM))
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
// disable matcher logs (because benchmark logs explodes)
#define ENABLE_MATCHER_DEBUG_LOGS       (0)


