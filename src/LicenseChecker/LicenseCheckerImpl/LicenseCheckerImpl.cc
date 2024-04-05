// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <sstream>
#include <cassert>
#include <nlohmann/json.hpp>
#include <restclient-cpp/restclient.h>
#include <restclient-cpp/connection.h>
#include <curl/curl.h>
#include "LicenseChecker.h"
#include "LicenseCheckerImpl.h"
#include "LicenseUtils.h"
#include "LicenseData.h"
#include "../../Logger/Logger.h"


static const char* LOG_TAG = "LicenseCheckerImpl";

// Json Serialize/Deserialize LicenseInfoResponse
using json = NLOHMANN_JSON_NAMESPACE::json;
using namespace NLOHMANN_JSON_NAMESPACE::literals;

namespace RealSenseID
{

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LicenseInfoResponse, license_type, payload)

namespace {
std::tuple<bool, std::string> GetRequestIdHeader(const RestClient::Response& response) {
    const std::string rid = "x-request-id";
    for (const auto& header: response.headers) {
        if (header.first.size() == rid.size()) {
            std::string header_lower(header.first);
            std::transform(header_lower.begin(), header_lower.end(), header_lower.begin(), ::tolower);
            if (header_lower == rid)
            {
                return std::make_tuple(true, header.second);
            }
        }
    }
    return std::make_tuple(false, std::string());
}

std::string url_encode(const std::string& decoded)
{
    const auto encoded_value = curl_easy_escape(nullptr, decoded.c_str(), static_cast<int>(decoded.length()));
    std::string result(encoded_value);
    curl_free(encoded_value);
    return result;
}

}

LicenseCheckStatus LicenseCheckerImpl::CheckLicense(const std::vector<unsigned char>& iv,
                                                    const std::vector<unsigned char>& enc_session_token,
                                                    const std::vector<unsigned char>& serial_number,
                                                    unsigned char* payload, int& license_type)
{        
    // Read license from storage
    std::string license_key;
    auto status = LicenseUtils::GetInstance().GetLicenseKey(license_key);
    if (!status.IsOk()) {
        LOG_ERROR(LOG_TAG, "Failed to read license key. Cannot proceed with license verification.");
        return LicenseCheckStatus::Error;
    }

    // Prepare encrypted session bundle
    // init with iv first
    std::vector<unsigned char> encrypted_session_bundle(iv);
    // append encrypted session token
    encrypted_session_bundle.insert(encrypted_session_bundle.end(), enc_session_token.begin(), enc_session_token.end());
    // encode for safe json
    const auto encoded = LicenseUtils::GetInstance().Base64Encode(encrypted_session_bundle);

    // Read serial number
    std::stringstream serial_stream;
    for (auto it :serial_number) {
        if (it == '\0') {   // If serial number is less than SERIAL_NUMBER_SIZE, don't send nulls to server
            break;
        }
        serial_stream << it;
    }

    std::stringstream url;
    url << LicenseUtils::GetInstance().GetLicenseEndpointUrl() << "?";
    url << "license_key" << "=" << url_encode(license_key) << "&";
    url << "serial_number" << "=" << url_encode(serial_stream.str()) << "&";
    url << "encrypted_session_token" << "=" << url_encode(encoded);

    // Send request
    RestClient::Response response;
    try {
        // create connection
        auto conn = std::make_unique<RestClient::Connection>("");

        // set headers
        RestClient::HeaderFields headers;
        headers["Accept"] = "application/json";
        conn->SetHeaders(headers);

        // set timeout
        conn->SetTimeout(10);
        conn->FollowRedirects(true, 1);

        // send request
        LOG_INFO(LOG_TAG, "GET license request to %s", url.str().c_str());
        response = conn->get(url.str());
        LOG_INFO(LOG_TAG, "HTTP response code %d ", response.code);
    } catch (const std::exception& ex) {
        LOG_ERROR(LOG_TAG, "Error while sending license request. Exception: %s", ex.what());
        return LicenseCheckStatus::Error;
    };

    bool request_id_found;
    std::string request_id;
    std::tie(request_id_found, request_id) = GetRequestIdHeader(response);

    if (request_id_found)
    {
        LOG_DEBUG(LOG_TAG, "License x-request-id: %s", request_id.c_str());
    }
    else
    {
        LOG_DEBUG(LOG_TAG, "License x-request-id: not found.");
    }

    if (response.code == -1) {
        LOG_ERROR(LOG_TAG, "Network issues");
        return LicenseCheckStatus::NetworkError;
    } else if (response.code == 28) {
        LOG_ERROR(LOG_TAG, "Network/gateway timeout.");
        return LicenseCheckStatus::NetworkError;
    } else if (response.code != 200) {        
        LOG_ERROR(LOG_TAG, "Response code %d", response.code);
        return LicenseCheckStatus::Error;
    }    
    std::vector<unsigned char> decoded_payload;
    // decoded_payload.size() == 429 ?
    size_t max_payload_size = LICENSE_VERIFICATION_RES_SIZE + LICENSE_SIGNATURE_SIZE;
    try
    {
        json j = json::parse(response.body);
        auto license_info = j.template get<LicenseInfoResponse>();
        decoded_payload = LicenseUtils::GetInstance().Base64Decode(license_info.payload);

        // Verify max size before filling thhe response buffer        
        if (decoded_payload.size() > max_payload_size)
        {
            LOG_ERROR(LOG_TAG, "Got invalid size %zu. Expected: %zu", decoded_payload.size(), max_payload_size);
            return LicenseCheckStatus::Error;
        }

        license_type = license_info.license_type;
        LOG_DEBUG(LOG_TAG, "License type: %d", license_info.license_type);
        switch (license_info.license_type)
        {
        case LicenseType::NO_FEATURES:
            LOG_WARNING(LOG_TAG, "License does not allow Facial Auth or Anti-spoof functionality.");
            break;
        case LicenseType::FACIAL_AUTH_SUBSCRIPTION:
        case LicenseType::FACIAL_AUTH_RENEWAL:
            LOG_INFO(LOG_TAG, "Subscription status: Facial Auth only.");
            break;
        case LicenseType::FACIAL_AUTH_PERPETUAL:
            LOG_INFO(LOG_TAG, "Perpetual status: Facial Auth only.");
            break;
        case LicenseType::ANTI_SPOOF_SUBSCRIPTION:
        case LicenseType::ANTI_SPOOF_RENEWAL:
            LOG_INFO(LOG_TAG, "Subscription status: Facial Auth and Anti-spoof functionality");
            break;
        case LicenseType::ANTI_SPOOF_PERPETUAL:
            LOG_INFO(LOG_TAG, "Perpetual status: Facial Auth and Anti-spoof.");
            break;
        default:
            LOG_ERROR(LOG_TAG, "Unknown license type.");
            break;
        }
        DEBUG_SERIAL(LOG_TAG, "License payload", encrypted_session_bundle.data(), encrypted_session_bundle.size());        
    } catch (const std::exception& ex) {
        LOG_ERROR(LOG_TAG, "Error while parsing license response. Exception: %s", ex.what());
        return LicenseCheckStatus::Error;
    }
    
    assert(decoded_payload.size() <= max_payload_size);
    std::memcpy(payload, decoded_payload.data(), decoded_payload.size());
    return LicenseCheckStatus::SUCCESS;
}

} // namespace RealSenseID
