// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/FaceAuthenticator.h"
#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <string.h>

// map of user-id->faceprint to demonstrate faceprints feature.
static std::map<std::string, RealSenseID::Faceprints> s_user_faceprint_db;


// Create FaceAuthenticator (after successfully connecting it to the device).
// If failed to connect, exit(1)
std::unique_ptr<RealSenseID::FaceAuthenticator> CreateAuthenticator(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = std::make_unique<RealSenseID::FaceAuthenticator>();
    auto connect_status = authenticator->Connect(serial_config);
    if (connect_status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed connecting to port " << serial_config.port << " status:" << connect_status << std::endl;
        std::exit(1);
    }
    std::cout << "Connected to device" << std::endl;
    return authenticator;
}

// extract faceprints for new enrolled user
class EnrollClbk : public RealSenseID::EnrollFaceprintsExtractionCallback
{
    std::string _user_id;

public:
    EnrollClbk(const char* user_id) : _user_id(user_id)
    {
    }

    void OnResult(const RealSenseID::EnrollStatus status, const RealSenseID::Faceprints* faceprints) override
    {
        std::cout << "on_result: status: " << status << std::endl;
        if (status == RealSenseID::EnrollStatus::Success)
        {
            s_user_faceprint_db[_user_id].version = faceprints->version;
            s_user_faceprint_db[_user_id].numberOfDescriptors = faceprints->numberOfDescriptors;
            static_assert(sizeof(s_user_faceprint_db[_user_id].avgDescriptor) == sizeof(faceprints->avgDescriptor),
                          "faceprints sizes does not match");
            ::memcpy(s_user_faceprint_db[_user_id].avgDescriptor, faceprints->avgDescriptor,
                     sizeof(faceprints->avgDescriptor));
        }
    }

    void OnProgress(const RealSenseID::FacePose pose) override
    {
        std::cout << "on_progress: pose: " << pose << std::endl;
    }

    void OnHint(const RealSenseID::EnrollStatus hint) override
    {
        std::cout << "on_hint: hint: " << hint << std::endl;
    }

};


void enroll_faceprints(const RealSenseID::SerialConfig& serial_config, const char* user_id)
{
    auto authenticator = CreateAuthenticator(serial_config);
    EnrollClbk enroll_clbk {user_id};
    RealSenseID::Faceprints fp;
    auto status = authenticator->ExtractFaceprintsForEnroll(enroll_clbk);
    std::cout << "Status: " << status << std::endl << std::endl;
}

// authenticate with faceprints
class FaceprintsAuthClbk : public RealSenseID::AuthFaceprintsExtractionCallback
{
    RealSenseID::FaceAuthenticator* _authenticator;

public:
    FaceprintsAuthClbk(RealSenseID::FaceAuthenticator* authenticator) : _authenticator(authenticator)
    {
    }

    void OnResult(const RealSenseID::AuthenticateStatus status, const RealSenseID::Faceprints* faceprints) override
    {
        std::cout << "on_result: status: " << status << std::endl;

        if (status != RealSenseID::AuthenticateStatus::Success)
        {
            std::cout << "ExtractFaceprints failed with status " << status << std::endl;
            return;
        }

        RealSenseID::Faceprints scanned_faceprint;
        scanned_faceprint.version = faceprints->version;
        scanned_faceprint.numberOfDescriptors = faceprints->numberOfDescriptors;
        static_assert(sizeof(scanned_faceprint.avgDescriptor) == sizeof(faceprints->avgDescriptor),
                      "faceprints sizes does not match");
        ::memcpy(scanned_faceprint.avgDescriptor, faceprints->avgDescriptor, sizeof(faceprints->avgDescriptor));

        // try to match the resulting faceprint to one of the faceprints stored in the db
        RealSenseID::Faceprints updated_faceprint;
        std::cout << "\nSearching " << s_user_faceprint_db.size() << " faceprints" << std::endl;
        for (auto& iter : s_user_faceprint_db)
        {
            auto& user_id = iter.first;
            auto& existing_faceprint = iter.second;
            auto match = _authenticator->MatchFaceprints(scanned_faceprint, existing_faceprint, updated_faceprint);
            if (match.success)
            {
                std::cout << "\n******* Match success. user_id: " << user_id << " *******\n" << std::endl;
                if (match.should_update)
                {
                    iter.second = updated_faceprint;
                    std::cout << "Updated faceprint in db.." << std::endl;
                }
                break;
            }
            else
            {
                std::cout << "\n******* Forbidden (no faceprint matched) *******\n" << std::endl;
            }
        }
    }

    void OnHint(const RealSenseID::AuthenticateStatus hint) override
    {
        std::cout << "on_hint: hint: " << hint << std::endl;
    }

      void OnFaceDetected(const std::vector<RealSenseID::FaceRect>& faces) override
    {
        for (auto& face : faces)
        {
            std::cout << "Detected face " << face.x << "," << face.y << " " << face.w << "x" << face.h << std::endl;
        }
    }
};


void authenticate_faceprints(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = CreateAuthenticator(serial_config);
    FaceprintsAuthClbk clbk(authenticator.get());
    RealSenseID::Faceprints scanned_faceprint;    
    // extract faceprints of the user in front of the device
    auto status = authenticator->ExtractFaceprintsForAuth(clbk);
    if (status != RealSenseID::Status::Ok)
        std::cout << "Status: " << status << std::endl << std::endl;
}


int main()
{
    RealSenseID::SerialConfig config {"COM9"};
    enroll_faceprints(config, "my-username");
    authenticate_faceprints(config);    
}
