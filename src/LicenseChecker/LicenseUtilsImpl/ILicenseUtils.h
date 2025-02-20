// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <string>
#include <utility>

namespace RealSenseID
{

static const std::string DEFAULT_LICENSE_SERVER_URL = "https://visionplatform.intelrealsense.com/v2/f450/license-status";

class LicenseResult
{
public:
    enum class Status
    {
        Ok = 100, /**< Operation succeeded */
        Error,    /**< General error */
    } status;

    LicenseResult() = delete;
    ~LicenseResult() = default;

    inline LicenseResult(Status status, std::string message) : status(status), message(std::move(message)) {};
    [[nodiscard]] inline std::string GetMessage() const
    {
        return message;
    };
    [[nodiscard]] inline bool IsOk() const
    {
        return status == Status::Ok;
    };

private:
    std::string message;
};

class ILicenseUtils
{
public:
    virtual ~ILicenseUtils() = default;
    virtual LicenseResult GetLicenseKey(std::string& license_key) = 0;
    virtual LicenseResult SetLicenseKey(const std::string& license_key, bool persist) = 0;
    virtual std::string GetLicenseEndpointUrl() = 0;
};

} // namespace RealSenseID