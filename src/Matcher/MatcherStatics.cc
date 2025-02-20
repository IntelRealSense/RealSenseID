#include "MatcherImplDefines.h"

using match_calc_t = short;

static const match_calc_t s_maxFeatureValue = static_cast<match_calc_t>(RSID_MAX_FEATURE_VALUE);
static const match_calc_t s_minFeatureValue = static_cast<match_calc_t>(RSID_MIN_FEATURE_VALUE);
static const match_calc_t s_minPossibleScore = static_cast<match_calc_t>(RSID_MIN_POSSIBLE_SCORE);

//======================================================================================
// 3 Sets of thresholds, with respect to confidence level (Low, Medium, High)
//======================================================================================
static const match_calc_t s_identicalThreshold_gNMgNM_HighConfLevel =
    static_cast<match_calc_t>(RSID_IDENTICAL_THRESHOLD_GNM_GNM_HIGH_CONFIDENCE_LEVEL);
static const match_calc_t s_identicalThreshold_gMgNM_HighConfLevel =
    static_cast<match_calc_t>(RSID_IDENTICAL_THRESHOLD_GM_GNM_HIGH_CONFIDENCE_LEVEL);
//---
static const match_calc_t s_strongThreshold_pNMgNM_HighConfLevel =
    static_cast<match_calc_t>(RSID_STRONG_THRESHOLD_PNM_GNM_HIGH_CONFIDENCE_LEVEL);
static const match_calc_t s_strongThreshold_pNMgNM_rgbImgEnroll_HighConfLevel =
    static_cast<match_calc_t>(RSID_STRONG_THRESHOLD_PNM_GNM_RGB_IMG_ENROLL_HIGH_CONFIDENCE_LEVEL);
static const match_calc_t s_strongThreshold_pMgM_HighConfLevel =
    static_cast<match_calc_t>(RSID_STRONG_THRESHOLD_PM_GM_HIGH_CONFIDENCE_LEVEL);
static const match_calc_t s_strongThreshold_pMgNM_HighConfLevel =
    static_cast<match_calc_t>(RSID_STRONG_THRESHOLD_PM_GNM_HIGH_CONFIDENCE_LEVEL);
//---
static const match_calc_t s_updateThreshold_pNMgNM_HighConfLevel =
    static_cast<match_calc_t>(RSID_UPDATE_THRESHOLD_PNM_GNM_HIGH_CONFIDENCE_LEVEL);
static const match_calc_t s_updateThreshold_pMgM_HighConfLevel =
    static_cast<match_calc_t>(RSID_UPDATE_THRESHOLD_PM_GM_HIGH_CONFIDENCE_LEVEL);
static const match_calc_t s_updateThreshold_pMgNM_First_HighConfLevel =
    static_cast<match_calc_t>(RSID_UPDATE_THRESHOLD_PM_GNM_FIRST_HIGH_CONFIDENCE_LEVEL);
//======================================================================================
static const match_calc_t s_identicalThreshold_gNMgNM_MediumConfLevel =
    static_cast<match_calc_t>(RSID_IDENTICAL_THRESHOLD_GNM_GNM_MEDIUM_CONFIDENCE_LEVEL);
static const match_calc_t s_identicalThreshold_gMgNM_MediumConfLevel =
    static_cast<match_calc_t>(RSID_IDENTICAL_THRESHOLD_GM_GNM_MEDIUM_CONFIDENCE_LEVEL);
//---
static const match_calc_t s_strongThreshold_pNMgNM_MediumConfLevel =
    static_cast<match_calc_t>(RSID_STRONG_THRESHOLD_PNM_GNM_MEDIUM_CONFIDENCE_LEVEL);
static const match_calc_t s_strongThreshold_pNMgNM_rgbImgEnroll_MediumConfLevel =
    static_cast<match_calc_t>(RSID_STRONG_THRESHOLD_PNM_GNM_RGB_IMG_ENROLL_MEDIUM_CONFIDENCE_LEVEL);
static const match_calc_t s_strongThreshold_pMgM_MediumConfLevel =
    static_cast<match_calc_t>(RSID_STRONG_THRESHOLD_PM_GM_MEDIUM_CONFIDENCE_LEVEL);
static const match_calc_t s_strongThreshold_pMgNM_MediumConfLevel =
    static_cast<match_calc_t>(RSID_STRONG_THRESHOLD_PM_GNM_MEDIUM_CONFIDENCE_LEVEL);
//---
static const match_calc_t s_updateThreshold_pNMgNM_MediumConfLevel =
    static_cast<match_calc_t>(RSID_UPDATE_THRESHOLD_PNM_GNM_MEDIUM_CONFIDENCE_LEVEL);
static const match_calc_t s_updateThreshold_pMgM_MediumConfLevel =
    static_cast<match_calc_t>(RSID_UPDATE_THRESHOLD_PM_GM_MEDIUM_CONFIDENCE_LEVEL);
static const match_calc_t s_updateThreshold_pMgNM_First_MediumConfLevel =
    static_cast<match_calc_t>(RSID_UPDATE_THRESHOLD_PM_GNM_FIRST_MEDIUM_CONFIDENCE_LEVEL);
//======================================================================================
static const match_calc_t s_identicalThreshold_gNMgNM_LowConfLevel =
    static_cast<match_calc_t>(RSID_IDENTICAL_THRESHOLD_GNM_GNM_LOW_CONFIDENCE_LEVEL);
static const match_calc_t s_identicalThreshold_gMgNM_LowConfLevel =
    static_cast<match_calc_t>(RSID_IDENTICAL_THRESHOLD_GM_GNM_LOW_CONFIDENCE_LEVEL);
//---
static const match_calc_t s_strongThreshold_pNMgNM_LowConfLevel =
    static_cast<match_calc_t>(RSID_STRONG_THRESHOLD_PNM_GNM_LOW_CONFIDENCE_LEVEL);
static const match_calc_t s_strongThreshold_pNMgNM_rgbImgEnroll_LowConfLevel =
    static_cast<match_calc_t>(RSID_STRONG_THRESHOLD_PNM_GNM_RGB_IMG_ENROLL_LOW_CONFIDENCE_LEVEL);
static const match_calc_t s_strongThreshold_pMgM_LowConfLevel = static_cast<match_calc_t>(RSID_STRONG_THRESHOLD_PM_GM_LOW_CONFIDENCE_LEVEL);
static const match_calc_t s_strongThreshold_pMgNM_LowConfLevel =
    static_cast<match_calc_t>(RSID_STRONG_THRESHOLD_PM_GNM_LOW_CONFIDENCE_LEVEL);
//---
static const match_calc_t s_updateThreshold_pNMgNM_LowConfLevel =
    static_cast<match_calc_t>(RSID_UPDATE_THRESHOLD_PNM_GNM_LOW_CONFIDENCE_LEVEL);
static const match_calc_t s_updateThreshold_pMgM_LowConfLevel = static_cast<match_calc_t>(RSID_UPDATE_THRESHOLD_PM_GM_LOW_CONFIDENCE_LEVEL);
static const match_calc_t s_updateThreshold_pMgNM_First_LowConfLevel =
    static_cast<match_calc_t>(RSID_UPDATE_THRESHOLD_PM_GNM_FIRST_LOW_CONFIDENCE_LEVEL);
//======================================================================================
