// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2024 Intel Corporation. All Rights Reserved.

#include "RealSenseID/UpdateChecker.h"
#include "rsid_c/rsid_update_checker.h"

#include <string.h>

#ifdef _MSC_VER    
    #define _RSID_STRDUP ::_strdup // avoid warning C4996 in visual studio
#else
    #define _RSID_STRDUP ::strdup
#endif

static void fill_result_struct(RealSenseID::UpdateCheck::ReleaseInfo& release_info, rsid_release_info* result)
{
    ::memset(result, 0, sizeof(rsid_release_info));
    result->sw_version = release_info.sw_version;
    result->fw_version = release_info.fw_version;
    if (release_info.sw_version_str != nullptr)
    {
        result->sw_version_str = _RSID_STRDUP(release_info.sw_version_str);
    }
    if (release_info.fw_version_str != nullptr)
    {
        result->fw_version_str = _RSID_STRDUP(release_info.fw_version_str);
    }
    if (release_info.release_url != nullptr)
    {
        result->release_url = _RSID_STRDUP(release_info.release_url);
    }
    if (release_info.release_notes_url != nullptr)
    {
        result->release_notes_url = _RSID_STRDUP(release_info.release_notes_url);
    }
}

void rsid_free_release_info(rsid_release_info* release_info)
{
    ::free((void*)release_info->sw_version_str);
    ::free((void*)release_info->fw_version_str);
    ::free((void*)release_info->release_url);
    ::free((void*)release_info->release_notes_url);
    ::memset(release_info, 0, sizeof(rsid_release_info));
}

rsid_status rsid_get_remote_release_info(rsid_release_info* result)
{
    RealSenseID::UpdateCheck::UpdateChecker checker;
    RealSenseID::UpdateCheck::ReleaseInfo release_info;
    auto status = checker.GetRemoteReleaseInfo(release_info);
    if (status == RealSenseID::Status::Ok)
	{		
	    fill_result_struct(release_info, result);	
	}    
    return static_cast<rsid_status>(status);
}


rsid_status rsid_get_local_release_info(const rsid_serial_config* serial_config, rsid_release_info* result)
{
    RealSenseID::UpdateCheck::UpdateChecker checker;
    RealSenseID::UpdateCheck::ReleaseInfo release_info;
    RealSenseID::SerialConfig serial_config_internal;
    serial_config_internal.port = serial_config->port;
    auto status = checker.GetLocalReleaseInfo(serial_config_internal, release_info); 
    if (status == RealSenseID::Status::Ok)
    {
        fill_result_struct(release_info, result);
    }    
    return static_cast<rsid_status>(status);
}