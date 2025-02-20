// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <memory>
#include <vector>
#include "LicenseUtilsImpl/ILicenseUtils.h"

namespace RealSenseID
{

template <typename T>
class Singleton
{
public:
    static T& GetInstance();

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton) = delete;

protected:
    Singleton() = default;
};

template <typename T>
T& Singleton<T>::GetInstance()
{
    static const std::unique_ptr<T> instance(new T());
    return *instance;
}


class LicenseUtils final : ILicenseUtils, public Singleton<LicenseUtils>
{
public:
    LicenseUtils();

    /**
     * @brief GetLicenseKey Returns license key
     * @param license_key The license key is stored here upon success
     *                    nullptr on failure.
     *                    If SetLicenseKey has been called, the value set in it will be returned until SetLicenseKey
     *                    is called with empty string. Otherwise, the value will be read from storage.
     * @return LicenseResult
     **/
    LicenseResult GetLicenseKey(std::string& license_key) override;

    /**
     * @brief SetLicenseKey: Sets license key to override the persisted (saved) license key.
     * @param license_key The license key you want to set.
     *                    Send empty string to undo and make GetLicenseKey read from persisted value.
     * @param persist Save the license key (for platforms that support it).
     * @return LicenseResult
     **/
    LicenseResult SetLicenseKey(const std::string& license_key, bool persist) override;

    /**
     * @brief GetLicenseEndpointUrl Returns license server endpoint URL
     * @return endpoint URL
     **/
    std::string GetLicenseEndpointUrl() override;


    std::string Base64Encode(const std::vector<unsigned char>& vec);
    std::vector<unsigned char> Base64Decode(const std::string& str);

private:
    std::unique_ptr<ILicenseUtils> licenseManagerImpl;
};

} // namespace RealSenseID
