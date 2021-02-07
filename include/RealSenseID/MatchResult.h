/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2011-2020 Intel Corporation. All Rights Reserved.
*******************************************************************************/
#pragma once
#include "RealSenseIDExports.h"

namespace RealSenseID
{
	/**
	* Result used by the Matcher module.
	*/
	struct RSID_API MatchResult
	{		
		bool success = false;
		bool should_update = false;		
	};
}