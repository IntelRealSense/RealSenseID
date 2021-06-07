// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

// Command line interface to RealSenseID device.
// Usage: rsid-cli <port> <usb/uart>.

#include "RealSenseID/FaceAuthenticator.h"
#include "RealSenseID/Preview.h"
#include "RealSenseID/DeviceController.h"
#include "RealSenseID/SignatureCallback.h"
#include "RealSenseID/Version.h"
#include "RealSenseID/Logging.h"
#include "RealSenseID/Faceprints.h"
#include <chrono>
#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cstdio>
#include <memory>
#include <map>
#include <set>
#include <thread>

#ifdef RSID_SECURE
#include "secure_mode_helper.h"
// signer object to store public keys of the host and device
static RealSenseID::Examples::SignHelper s_signer;
#endif // RSID_SECURE

// map of user-id->faceprint_pair to demonstrate faceprints feature.
static std::map<std::string, RealSenseID::Faceprints> s_user_faceprint_db;

// last faceprint auth extract status
static RealSenseID::AuthenticateStatus s_last_auth_faceprint_status;

// last faceprint enroll extract status
static RealSenseID::EnrollStatus s_last_enroll_faceprint_status;


// Create FaceAuthenticator (after successfully connecting it to the device).
// If failed to connect, exit(1)
std::unique_ptr<RealSenseID::FaceAuthenticator> CreateAuthenticator(const RealSenseID::SerialConfig& serial_config)
{
#ifdef RSID_SECURE
    auto authenticator = std::make_unique<RealSenseID::FaceAuthenticator>(&s_signer);
#else
    auto authenticator = std::make_unique<RealSenseID::FaceAuthenticator>();
#endif // RSID_SECURE
    auto connect_status = authenticator->Connect(serial_config);
    if (connect_status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed connecting to port " << serial_config.port << " status:" << connect_status << std::endl;
        std::exit(1);
    }
    std::cout << "Connected to device" << std::endl;
    return authenticator;
}


#ifdef RSID_PREVIEW

class PreviewRender : public RealSenseID::PreviewImageReadyCallback
{
public:
    void OnPreviewImageReady(const RealSenseID::Image image)
    {
        std::cout << "\rframe #" << image.number << ": " << image.width << "x" << image.height << " (" << image.size
                  << " bytes)" <<std::endl;

        // Enable this code to enable saving images as ppm files
        //
        //    std::string filename = "outputimage" +  std::to_string(image.number) + ".ppm";
        //    FILE* f1 = fopen(filename.c_str(), "wb");
        //    fprintf(f1, "P6\n%d %d\n255\n", image.width, image.height);
        //    fwrite(image.buffer, 1, image.size, f1);
        //    fclose(f1);
        //
    }
};

std::unique_ptr<RealSenseID::Preview> _preview;
std::unique_ptr<PreviewRender> _preview_callback;

#endif


class MyEnrollClbk : public RealSenseID::EnrollmentCallback
{
    using FacePose = RealSenseID::FacePose;

public:
    void OnResult(const RealSenseID::EnrollStatus status) override
    {
        // std::cout << "on_result: status: " << status << std::endl;
        std::cout << "  *** Result " << status << std::endl;
    }

    void OnProgress(const FacePose pose) override
    {
        // find next pose that required(if any) and display instruction where to look
        std::cout << "  *** Detected Pose " << pose << std::endl;
        _poses_required.erase(pose);
        if (!_poses_required.empty())
        {
            auto next_pose = *_poses_required.begin();
            std::cout << "  *** Please Look To The " << next_pose << std::endl;
        }
    }

    void OnHint(const RealSenseID::EnrollStatus hint) override
    {
        std::cout << "  *** Hint " << hint << std::endl;
    }

private:
    std::set<RealSenseID::FacePose> _poses_required = {FacePose::Center, FacePose::Left, FacePose::Right};
};

void do_enroll(const RealSenseID::SerialConfig& serial_config, const char* user_id)
{
    auto authenticator = CreateAuthenticator(serial_config);
    MyEnrollClbk enroll_clbk;
    auto status = authenticator->Enroll(enroll_clbk, user_id);
    if (status != RealSenseID::Status::Ok)
    {
        std::cout << "Status: " << status << std::endl << std::endl;
    }
}

// Authentication callback
class MyAuthClbk : public RealSenseID::AuthenticationCallback
{
public:
    void OnResult(const RealSenseID::AuthenticateStatus status, const char* user_id) override
    {
        if (status == RealSenseID::AuthenticateStatus::Success)
        {
            std::cout << "******* Authenticate success.  user_id: " << user_id << " *******" << std::endl;
        }
        else
        {
            std::cout << "on_result: status: " << status << std::endl;
        }
    }

    void OnHint(const RealSenseID::AuthenticateStatus hint) override
    {
        std::cout << "on_hint: hint: " << hint << std::endl;
    }
};

void do_authenticate(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = CreateAuthenticator(serial_config);
    MyAuthClbk auth_clbk;
    auto status = authenticator->Authenticate(auth_clbk);
    if (status != RealSenseID::Status::Ok)
    {
        std::cout << "Status: " << status << std::endl << std::endl;
    }
}

void remove_users(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = CreateAuthenticator(serial_config);
    auto auth_status = authenticator->RemoveAll();
    std::cout << "Final status:" << auth_status << std::endl << std::endl;
}

#ifdef RSID_SECURE
void pair_device(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = CreateAuthenticator(serial_config);
    char* host_pubkey = (char*)s_signer.GetHostPubKey();
    char host_pubkey_signature[32] = {0};
    char device_pubkey[64] = {0};
    auto pair_status = authenticator->Pair(host_pubkey, host_pubkey_signature, device_pubkey);
    if (pair_status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed pairing with device" << std::endl;
        return;
    }
    s_signer.UpdateDevicePubKey((unsigned char*)device_pubkey);
    std::cout << "Final status:" << pair_status << std::endl << std::endl;
}

void unpair_device(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = CreateAuthenticator(serial_config);
    auto unpair_status = authenticator->Unpair();
    if (unpair_status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed to unpair with device" << std::endl;
        return;
    }
    std::cout << "Final status:" << unpair_status << std::endl << std::endl;
}
#endif // RSID_SECURE

// SetDeviceConfig
void set_device_config(const RealSenseID::SerialConfig& serial_config, RealSenseID::DeviceConfig& device_config)
{
    auto authenticator = CreateAuthenticator(serial_config);
    auto status = authenticator->SetDeviceConfig(device_config);
    std::cout << "Status: " << status << std::endl << std::endl;
}

void get_device_config(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = CreateAuthenticator(serial_config);
    RealSenseID::DeviceConfig device_config;
    auto status = authenticator->QueryDeviceConfig(device_config);
    if (status == RealSenseID::Status::Ok)
    {
        std::cout << std::endl << "Authentication settings::" << std::endl;
        std::cout << " * Rotation: " << device_config.camera_rotation << std::endl;
        std::cout << " * Security: " << device_config.security_level << std::endl;        
        std::cout << " * Algo flow Mode: " << device_config.algo_flow << std::endl;
        std::cout << " * Face policy : " << device_config.face_selection_policy << std::endl;
        std::cout << " * Preview Mode: " << device_config.preview_mode << std::endl;
        std::cout << " * Dump Mode: " << device_config.dump_mode << std::endl;
    }
    else
    {
        std::cout << "Status: " << status << std::endl << std::endl;
    }
}

void get_number_users(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = CreateAuthenticator(serial_config);

    unsigned int number_of_users = 0;
    auto status = authenticator->QueryNumberOfUsers(number_of_users);
    if (status == RealSenseID::Status::Ok)
    {
        std::cout << "Number of users: " << number_of_users << std::endl << std::endl;
    }
    else
    {
        std::cout << "Status: " << status << std::endl << std::endl;
    }
}

void get_users(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = CreateAuthenticator(serial_config);


    unsigned int number_of_users = 0;
    auto status = authenticator->QueryNumberOfUsers(number_of_users);
    if (status != RealSenseID::Status::Ok)
    {
        std::cout << "Status: " << status << std::endl << std::endl;
        return;
    }

    if (number_of_users == 0)
    {
        std::cout << "No users found" << std::endl << std::endl;
        return;
    }
    // allocate needed array of user ids
    char** user_ids = new char*[number_of_users];
    for (unsigned i = 0; i < number_of_users; i++)
    {
        user_ids[i] = new char[RealSenseID::FaceAuthenticator::MAX_USERID_LENGTH];
    }
    unsigned int nusers_in_out = number_of_users;
    status = authenticator->QueryUserIds(user_ids, nusers_in_out);
    if (status != RealSenseID::Status::Ok)
    {
        std::cout << "Status: " << status << std::endl << std::endl;
        // free allocated memory and return on error
        for (unsigned int i = 0; i < number_of_users; i++)
        {
            delete user_ids[i];
        }
        delete[] user_ids;
        return;
    }

    std::cout << std::endl << nusers_in_out << " Users:\n==========\n";
    for (unsigned int i = 0; i < nusers_in_out; i++)
    {
        std::cout << (i + 1) << ".  " << user_ids[i] << std::endl;
    }

    std::cout << std::endl;

    // free allocated memory
    for (unsigned int i = 0; i < number_of_users; i++)
    {
        delete user_ids[i];
    }
    delete[] user_ids;
}

void standby_db_save(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = CreateAuthenticator(serial_config);

    unsigned int number_of_users = 0;
    auto status = authenticator->Standby();
    std::cout << "Status: " << status << std::endl << std::endl;
}

void device_info(const RealSenseID::SerialConfig& serial_config)
{
    RealSenseID::DeviceController deviceController;
    auto connect_status = deviceController.Connect(serial_config);
    if (connect_status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed connecting to port " << serial_config.port << " status:" << connect_status << std::endl;
        return;
    }

    std::string firmware_version;
    auto status = deviceController.QueryFirmwareVersion(firmware_version);
    if (status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed getting firmware version!\n";
    }

    std::string serial_number;
    status = deviceController.QuerySerialNumber(serial_number);
    if (status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed getting serial number!\n";
    }

    deviceController.Disconnect();

    std::string host_version = RealSenseID::Version();

    std::cout << "\n";
    std::cout << "Additional information:\n";
    std::cout << " * S/N: " << serial_number << "\n";
    std::cout << " * Firmware: " << firmware_version << "\n";
    std::cout << " * Host: " << host_version << "\n";
    std::cout << "\n";
}

// ping X iterations and display roundtrip times
void ping_device(const RealSenseID::SerialConfig& serial_config, int iters)
{
    if (iters < 1)
    {
        return;
    }

    RealSenseID::DeviceController deviceController;
    auto connect_status = deviceController.Connect(serial_config);
    if (connect_status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed connecting to port " << serial_config.port << " status:" << connect_status << std::endl;
        return;
    }

    using clock = std::chrono::steady_clock;
    for (int i = 0; i < iters; i++)
    {
        auto start_time = clock::now();
        auto status = deviceController.Ping();
        auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start_time).count();
        printf("Ping #%04d %s. Roundtrip %03zu millis\n\n", (i + 1), RealSenseID::Description(status), elapsed_ms);
        if (status != RealSenseID::Status::Ok)
        {
            printf("Ping error\n\n");
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds {5});
    }
}

// extract faceprints for new enrolled user
class MyEnrollServerClbk : public RealSenseID::EnrollFaceprintsExtractionCallback
{
    std::string _user_id;

public:
    MyEnrollServerClbk(const char* user_id) : _user_id(user_id)
    {
    }

    void OnResult(const RealSenseID::EnrollStatus status, const RealSenseID::ExtractedFaceprints* faceprints) override
    {
        std::cout << "on_result: status: " << status << std::endl;

        if (status == RealSenseID::EnrollStatus::Success)
        {
            // handle with/without mask vectors properly.

            // set the full data for the enrolled object:
            s_user_faceprint_db[_user_id].data.version = faceprints->data.version;
            s_user_faceprint_db[_user_id].data.flags = faceprints->data.flags;
            s_user_faceprint_db[_user_id].data.featuresType = faceprints->data.featuresType;

            // During enroll we update both vectors (enrollment + adaptive).

            static_assert(sizeof(s_user_faceprint_db[_user_id].data.adaptiveDescriptorWithoutMask) == sizeof(faceprints->data.featuresVector), "adaptive faceprints vector (without mask) sizes does not match");
            ::memcpy(&s_user_faceprint_db[_user_id].data.adaptiveDescriptorWithoutMask[0], &faceprints->data.featuresVector[0], sizeof(faceprints->data.featuresVector));

            static_assert(sizeof(s_user_faceprint_db[_user_id].data.enrollmentDescriptor) == sizeof(faceprints->data.featuresVector), "enrollment faceprints vector sizes does not match");
            ::memcpy(&s_user_faceprint_db[_user_id].data.enrollmentDescriptor[0], &faceprints->data.featuresVector[0], sizeof(faceprints->data.featuresVector));        

            // mark the withMask vector as invalid because its not set yet!
            s_user_faceprint_db[_user_id].data.adaptiveDescriptorWithMask[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS] = RealSenseID::FaVectorFlagsEnum::VecFlagNotSet;
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
    MyEnrollServerClbk enroll_clbk {user_id};
    s_last_enroll_faceprint_status = RealSenseID::EnrollStatus::CameraStarted;
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

    void OnResult(const RealSenseID::AuthenticateStatus status, const RealSenseID::ExtractedFaceprints* faceprints) override
    {
        std::cout << "on_result: status: " << status << std::endl;

        if (status != RealSenseID::AuthenticateStatus::Success)
        {
            std::cout << "ExtractFaceprints failed with status " << s_last_auth_faceprint_status << std::endl
                      << std::endl;
            return;
        }

        RealSenseID::MatchElement scanned_faceprint;
        // scanned_faceprint.featuresType = faceprints->data.featuresType;
        scanned_faceprint.data.version = faceprints->data.version;
        scanned_faceprint.data.featuresType = faceprints->data.featuresType;
        scanned_faceprint.data.flags = faceprints->data.featuresVector[RSID_INDEX_IN_FEATURES_VECTOR_TO_FLAGS];
        static_assert(sizeof(scanned_faceprint.data.featuresVector) == sizeof(faceprints->data.featuresVector),
                      "new adaptive faceprints vector sizes does not match");
        ::memcpy(&scanned_faceprint.data.featuresVector[0], &faceprints->data.featuresVector[0],
                 sizeof(faceprints->data.featuresVector));

        // try to match the new faceprint to one of the faceprints stored in the db
        RealSenseID::Faceprints updated_faceprint;
        
        std::cout << "\nSearching " << s_user_faceprint_db.size() << " faceprints" << std::endl;
        
        int save_max_score = -1;
        int winning_index = -1;
        std::string winning_id_str = "";
        RealSenseID::MatchResultHost winning_match_result;
        RealSenseID::Faceprints winning_updated_faceprints;
        int users_index = 0;

        for (auto& iter : s_user_faceprint_db)
        {
            auto user_id = iter.first;

            RealSenseID::Faceprints existing_faceprint = iter.second;       // the previous vector from the DB.
            RealSenseID::Faceprints updated_faceprint = existing_faceprint; // init updated to previous state in the DB.

            auto match = _authenticator->MatchFaceprints(scanned_faceprint, existing_faceprint, updated_faceprint);

            int current_score = (int)match.score;

            // save the best winner that matched.
            if (match.success)
            {
                if(current_score > save_max_score)
                {
                    save_max_score = current_score;
                    winning_match_result = match;
                    winning_index = users_index;
                    winning_id_str = user_id;
                    winning_updated_faceprints = updated_faceprint;
                }

            }

            users_index++;

        } // end of for() loop

        if(winning_index >= 0) // we have a winner so declare success!
        {
            std::cout << "\n******* Match success. user_id: " << winning_id_str << " *******\n" << std::endl;
            // apply adaptive-update on the db.
            if (winning_match_result.should_update)
            {
                // apply adaptive update
                s_user_faceprint_db[winning_id_str] = winning_updated_faceprints; 
                std::cout << "DB adaptive apdate applied to user = " << winning_id_str << "." << std::endl;
            }
        }
        else // no winner, declare authentication failed!
        {
            std::cout << "\n******* Forbidden (no faceprint matched) *******\n" << std::endl;
        }

    }

    void OnHint(const RealSenseID::AuthenticateStatus hint) override
    {
        std::cout << "on_hint: hint: " << hint << std::endl;
    }
};

void authenticate_faceprints(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = CreateAuthenticator(serial_config);
    FaceprintsAuthClbk clbk(authenticator.get());
    s_last_auth_faceprint_status = RealSenseID::AuthenticateStatus::CameraStarted;
    // extract faceprints of the user in front of the device
    auto status = authenticator->ExtractFaceprintsForAuth(clbk);
    if (status != RealSenseID::Status::Ok)
        std::cout << "Status: " << status << std::endl << std::endl;
}

void print_usage()
{
    std::cout << "Usage: rsid-cli <port>" << std::endl;
}

RealSenseID::SerialConfig config_from_argv(int argc, char* argv[])
{
    RealSenseID::SerialConfig config;
    if (argc < 2)
    {
        print_usage();
        std::exit(1);
    }
    config.port = argv[1];

    return config;
}

void print_menu_opt(const char* line)
{
    printf("  %s\n", line);
}

void print_menu()
{
    printf("Please select an option:\n\n");
    print_menu_opt("'e' to enroll.");
    print_menu_opt("'a' to authenticate.");
    print_menu_opt("'d' to delete all users.");
#ifdef RSID_SECURE
    print_menu_opt("'p' to pair with the device (enables secure communication).");
    print_menu_opt("'i' to unpair with the device (disables secure communication).");
#endif // RSID_SECURE
#ifdef RSID_PREVIEW
    print_menu_opt("'c' to capture images from device.");
#endif // RSID_SECURE
    print_menu_opt("'s' to set authentication settings.");
    print_menu_opt("'g' to query authentication settings.");
    print_menu_opt("'u' to query ids of users.");
    print_menu_opt("'n' to query number of users.");
    print_menu_opt("'b' to save device's database before standby.");
    print_menu_opt("'v' to view additional information.");
    print_menu_opt("'x' to ping the device.");
    print_menu_opt("'q' to quit.");

    // server mode opts
    print_menu_opt("\nserver mode options:");
    print_menu_opt("'E' to enroll with faceprints.");
    print_menu_opt("'A' to authenticate with faceprints.");
    print_menu_opt("'U' to list enrolled users");
    print_menu_opt("'D' to delete all users.");
    printf("\n");
    printf("> ");
}

void sample_loop(const RealSenseID::SerialConfig& serial_config)
{
    bool is_running = true;
    std::string input;

    get_device_config(serial_config);

    while (is_running)
    {
        print_menu();

        if (!std::getline(std::cin, input))
            continue;

        if (input.empty() || input.length() > 1)
            continue;

        char key = input[0];

        switch (key)
        {
        case 'e': {
            std::string user_id;
            do
            {
                std::cout << "User id to enroll: ";
                std::getline(std::cin, user_id);
            } while (user_id.empty());
            do_enroll(serial_config, user_id.c_str());
            break;
        }
        case 'a':
            do_authenticate(serial_config);
            break;
        case 'd':
            remove_users(serial_config);
            break;
#ifdef RSID_SECURE
        case 'p':
            pair_device(serial_config);
            break;
        case 'i':
            unpair_device(serial_config);
            break;
#endif // RSID_SECURE
#ifdef RSID_PREVIEW
        case 'c': {
            RealSenseID::PreviewConfig config;
            _preview = std::make_unique<RealSenseID::Preview>(config);
            _preview_callback = std::make_unique<PreviewRender>();
            _preview->StartPreview(*_preview_callback);
            std::cout << "starting preview for 3 seconds ";
            std::this_thread::sleep_for(std::chrono::seconds {3});
            _preview->StopPreview();
            std::this_thread::sleep_for(std::chrono::milliseconds {400});
            std::cout << std::endl;
            break;
        }
#endif // RSID_SECURE
        case 's': {
            RealSenseID::DeviceConfig config;
            config.camera_rotation = RealSenseID::DeviceConfig::CameraRotation::Rotation_0_Deg;
            config.security_level = RealSenseID::DeviceConfig::SecurityLevel::High;
            std::string sec_level;
            std::cout << "Set security level(medium/high): ";
            std::getline(std::cin, sec_level);
            if (sec_level.find("med") != -1)
            {
                config.security_level = RealSenseID::DeviceConfig::SecurityLevel::Medium;
            }
            else
            {
                config.security_level = RealSenseID::DeviceConfig::SecurityLevel::High;
            }
            std::cout << "Set algo flow (all/detection/recognition/spoof only): ";
            std::getline(std::cin, sec_level);
            if (sec_level.find("rec") != -1)
            {
                config.algo_flow = RealSenseID::DeviceConfig::AlgoFlow::RecognitionOnly;            
            }
            else if (sec_level.find("spoof") != -1)
            {
                config.algo_flow = RealSenseID::DeviceConfig::AlgoFlow::SpoofOnly;            
            }
            else if (sec_level.find("detection") != -1)
            {
                config.algo_flow = RealSenseID::DeviceConfig::AlgoFlow::FaceDetectionOnly;            
            }
            else
            {
                config.algo_flow = RealSenseID::DeviceConfig::AlgoFlow::All;
            }
            
            std::string rot_level;
            std::cout << "Set rotation level(0/180): ";
            std::getline(std::cin, rot_level);
            if (rot_level.find("180") != std::string::npos)
            {
                config.camera_rotation = RealSenseID::DeviceConfig::CameraRotation::Rotation_180_Deg;
            }

            std::cout << "Set multiple face policy (single/all): ";
            std::getline(std::cin, sec_level);
            if (sec_level.find("all") != -1)
            {
                config.face_selection_policy = RealSenseID::DeviceConfig::FaceSelectionPolicy::All;            
            }
            else
            {
                config.face_selection_policy = RealSenseID::DeviceConfig::FaceSelectionPolicy::Single;              
            }

            set_device_config(serial_config, config);
            break;
        }
        case 'g':
            get_device_config(serial_config);
            break;
        case 'u':
            get_users(serial_config);
            break;
        case 'n':
            get_number_users(serial_config);
            break;
        case 'b':
            standby_db_save(serial_config);
            break;
        case 'v':
            device_info(serial_config);
            break;
        case 'x': {
            int iters = -1;
            std::string line;
            do
            {
                try
                {
                    std::cout << "Iterations:\n>>";
                    std::getline(std::cin, line);
                    iters = std::stoi(line);
                }
                catch (std::invalid_argument&)
                {
                }
            } while (iters < 0);
            ping_device(serial_config, iters);
            break;
        }
        case 'q':
            is_running = false;
            break;
        case 'E': {
            std::string user_id;
            do
            {
                std::cout << "User id to enroll: ";
                std::getline(std::cin, user_id);
            } while (user_id.empty());
            enroll_faceprints(serial_config, user_id.c_str());
            break;
        }
        case 'A':
            authenticate_faceprints(serial_config);
            break;
        case 'U': {
            std::cout << std::endl << s_user_faceprint_db.size() << " users\n";
            for (const auto& iter : s_user_faceprint_db)
            {
                std::cout << " * " << iter.first << std::endl;
            }
            std::cout << std::endl;
            break;
        }
        case 'D':
            s_user_faceprint_db.clear();
            std::cout << "\nFaceprints deleted..\n" << std::endl;
            break;
        }
    }
}

int main(int argc, char* argv[])
{
    auto config = config_from_argv(argc, argv);
    sample_loop(config);
    return 0;
}
