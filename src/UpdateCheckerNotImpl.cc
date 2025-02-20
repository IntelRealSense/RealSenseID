#include "RealSenseID/UpdateChecker.h"
#include "Logger.h"


static const char* LOG_TAG = "UpdateCheckerNotImpl";

namespace RealSenseID
{

Status UpdateCheck::UpdateChecker::GetRemoteReleaseInfo(ReleaseInfo& release_info) const
{
    LOG_ERROR(LOG_TAG, "GetRemoteReleaseInfo(..) not implemented");
    return Status::Error;
}

Status UpdateCheck::UpdateChecker::GetLocalReleaseInfo(const RealSenseID::SerialConfig& serial_config, ReleaseInfo& release_info) const
{
    LOG_ERROR(LOG_TAG, "GetLocalReleaseInfo(..) not implemented");
    return Status::Error;
}
} // namespace RealSenseID
