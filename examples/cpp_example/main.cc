// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/FaceAuthenticator.h"
#include "RealSenseID/Preview.h"
#include "RealSenseID/DeviceController.h"
#include "RealSenseID/SignatureCallback.h"
#include "RealSenseID/Version.h"
#include "RealSenseID/Logging.h"
#include "RealSenseID/Faceprints.h"
#include "rsid_signature_example.h"
#include <chrono>
#include <string>
#include <iostream>
#include <cstdio>
#include <memory>
#include <map>

// signer object to store public keys of the host and device (see example below on usage)
static RealSenseID::Examples::SignHelper s_signer;

// map of user-id->faceprint to demonstrate faceprints feature.
static std::map<std::string, RealSenseID::Faceprints> s_user_faceprint_db;

//last faceprint auth extract status
static RealSenseID::AuthenticateStatus s_last_auth_faceprint_status;

// last faceprint enroll extract status
static RealSenseID::EnrollStatus s_last_enroll_faceprint_status;
    //
// Enroll example
//
class MyEnrollClbk : public RealSenseID::EnrollmentCallback
{
public:
    void OnResult(const RealSenseID::EnrollStatus status) override
    {
        std::cout << "on_result: status: " << status << std::endl;
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


// Create FaceAuthenticator (after successfully connecting it to the device).
// If failed to connect, exit(1)
std::unique_ptr<RealSenseID::FaceAuthenticator> CreateAuthenticator(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = std::make_unique<RealSenseID::FaceAuthenticator>(&s_signer);
    auto connect_status = authenticator->Connect(serial_config);
    if (connect_status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed connecting to port " << serial_config.port << " status:" << connect_status << std::endl;
        std::exit(1);
    }
    std::cout << "Connected to device" << std::endl;
    return authenticator;
}

// Enroll example
void enroll_example(const RealSenseID::SerialConfig& serial_config, const char* user_id)
{
    auto authenticator = CreateAuthenticator(serial_config);
    MyEnrollClbk enroll_clbk;
    auto status = authenticator->Enroll(enroll_clbk, user_id);
    if (status != RealSenseID::Status::Ok)
    {
        std::cout << "Status: " << status << std::endl << std::endl;
    }
}

// SetAuthSettings example
void set_auth_settings_example(const RealSenseID::SerialConfig& serial_config, RealSenseID::AuthConfig& auth_config)
{
    auto authenticator = CreateAuthenticator(serial_config);
    auto status = authenticator->SetAuthSettings(auth_config);
    std::cout << "Status: " << status << std::endl << std::endl;
}

void get_auth_settings_example(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = CreateAuthenticator(serial_config);
    RealSenseID::AuthConfig auth_config;
    auto status = authenticator->QueryAuthSettings(auth_config);
    if (status == RealSenseID::Status::Ok)
    {
        std::cout << "\nAuthentication settings::\n";
        std::cout << " * Rotation: " << auth_config.camera_rotation << std::endl;
        std::cout << " * Security: " << auth_config.security_level << std::endl << std::endl;
    }
    else
    {
        std::cout << "Status: " << status << std::endl << std::endl;
    }
}

void get_number_users_example(const RealSenseID::SerialConfig& serial_config)
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

void standby_db_save(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = CreateAuthenticator(serial_config);

    unsigned int number_of_users = 0;
    auto status = authenticator->Standby();
    std::cout << "Status: " << status << std::endl << std::endl;
}

void get_users_example(const RealSenseID::SerialConfig& serial_config)
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
        user_ids[i] = new char[16];
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


// Pairing example
void pairing_example(const RealSenseID::SerialConfig& serial_config)
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

// Authentication example
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

void authenticate_example(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = CreateAuthenticator(serial_config);
    MyAuthClbk auth_clbk;
    auto status = authenticator->Authenticate(auth_clbk);
    if (status != RealSenseID::Status::Ok)
    {
        std::cout << "Status: " << status << std::endl << std::endl;
    }
}

void additional_information_example(const RealSenseID::SerialConfig& serial_config)
{
    RealSenseID::DeviceController deviceController;
    auto connect_status = deviceController.Connect(serial_config);
    if (connect_status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed connecting to port " << serial_config.port << " status:" << connect_status << std::endl;
        return;
    }

    std::string firmware_version;
    deviceController.QueryFirmwareVersion(firmware_version);
    std::string serial_number;
    deviceController.QuerySerialNumber(serial_number);
    deviceController.Disconnect();

    std::string host_version = RealSenseID::Version();

    std::cout << "\n";
    std::cout << "Additional information:\n";
    std::cout << " * S/N: " << serial_number << "\n";
    std::cout << " * Firmware: " << firmware_version << "\n";
    std::cout << " * Host: " << host_version << "\n";
    std::cout << "\n";
}

// Remove all users example
void remove_users_example(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = CreateAuthenticator(serial_config);
    auto auth_status = authenticator->RemoveAll();
    std::cout << "Final status:" << auth_status << std::endl << std::endl;
}

// ping X iterations and display roundtrip times
void ping_example(const RealSenseID::SerialConfig& serial_config, int iters)
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
    }
}

// extract faceprints for new enrolled user
class MyEnrollServerClbk : public RealSenseID::EnrollmentCallback
{
    const char* _user_id = nullptr;

public:
    explicit MyEnrollServerClbk(const char* user_id) : _user_id {user_id}
    {        
    }
    void OnResult(const RealSenseID::EnrollStatus status) override
    {
        std::cout << "on_result: status: " << status << std::endl;
        s_last_enroll_faceprint_status = status;               
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
void enroll_faceprint_example(const RealSenseID::SerialConfig& serial_config, const char* user_id)
{
    auto authenticator = CreateAuthenticator(serial_config);
    MyEnrollServerClbk enroll_clbk {user_id};
    RealSenseID::Faceprints fp;
    s_last_enroll_faceprint_status = RealSenseID::EnrollStatus::CameraStarted;
    auto status = authenticator->EnrollExtractFaceprints(enroll_clbk, user_id, fp);
    std::cout << "Status: " << status << std::endl << std::endl;
    if (status == RealSenseID::Status::Ok && s_last_enroll_faceprint_status == RealSenseID::EnrollStatus::Success)
    {
        // todo move this to callback once the api fixed        
        s_user_faceprint_db[user_id] = fp;
    }
}

// authenticate with faceprints example
class FaceprintsAuthClbk : public RealSenseID::AuthFaceprintsExtractionCallback
{
public:
    void OnResult(const RealSenseID::AuthenticateStatus status) override
    {
        std::cout << "on_result: status: " << status << std::endl;
        s_last_auth_faceprint_status = status;
    }

    void OnHint(const RealSenseID::AuthenticateStatus hint) override
    {
        std::cout << "on_hint: hint: " << hint << std::endl;
    }
};

void authenticate_faceprint_example(const RealSenseID::SerialConfig& serial_config)
{
    auto authenticator = CreateAuthenticator(serial_config);
    FaceprintsAuthClbk clbk;
    RealSenseID::Faceprints scanned_faceprint;
    s_last_auth_faceprint_status = RealSenseID::AuthenticateStatus::CameraStarted;    
    // extract faceprints of the user in front of the device
    auto status = authenticator->AuthenticateExtractFaceprints(clbk, scanned_faceprint);
    if (status != RealSenseID::Status::Ok)
    {
        std::cout << "Status: " << status << std::endl << std::endl;
        return;
    }

    if (s_last_auth_faceprint_status != RealSenseID::AuthenticateStatus::Success)
    {
        std::cout << "ExtractFaceprints failed with status " << s_last_auth_faceprint_status << std::endl << std::endl;
        return;
    }
    
    // try to match the resulting faceprint to one of the faceprints stored in the db
    RealSenseID::Faceprints updated_faceprint;
    std::cout << "\nSearching " << s_user_faceprint_db.size() << " faceprints" << std::endl;
    for (auto& iter : s_user_faceprint_db)
    {
        auto& user_id = iter.first;
        auto& existing_faceprint = iter.second;        
        auto match = authenticator->MatchFaceprints(scanned_faceprint, existing_faceprint, updated_faceprint);
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

void print_usage()
{
    std::cout << "Usage: rsid_cpp_example <port> <usb/uart>" << std::endl;
}

RealSenseID::SerialConfig config_from_argv(int argc, char* argv[])
{
    RealSenseID::SerialConfig config;
    if (argc < 3)
    {
        print_usage();
        std::exit(1);
    }
    config.port = argv[1];
    std::string ser_type = argv[2];
    if (ser_type == "usb" || ser_type == "USB")
    {
        config.serType = RealSenseID::SerialType::USB;
    }
    else if (ser_type == "uart")
    {
        config.serType = RealSenseID::SerialType::UART;
    }
    else
    {
        print_usage();
        std::exit(1);
    }
    return config;
}

void print_menu_opt(const char* line)
{
    printf("  %s\n", line);
}

void print_menu()
{
    printf("Please select an option:\n\n");
    print_menu_opt("'p' to pair with the device (must be performed at least once).");
    print_menu_opt("'a' to authenticate.");
    print_menu_opt("'e' to enroll.");
    print_menu_opt("'d' to delete all users.");
    print_menu_opt("'s' to set authentication settings.");
    print_menu_opt("'g' to query authentication settings.");
    print_menu_opt("'n' to query number of users.");
    print_menu_opt("'u' to query ids of users.");
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
            enroll_example(serial_config, user_id.c_str());
            break;
        }
        case 'a':
            authenticate_example(serial_config);
            break;

        case 'd':
            remove_users_example(serial_config);
            break;
        case 's': {
            RealSenseID::AuthConfig config;
            config.camera_rotation = RealSenseID::AuthConfig::CameraRotation::Rotation_0_Deg;
            config.security_level = RealSenseID::AuthConfig::SecurityLevel::High;
            std::string sec_level;
            std::cout << "Set security level(medium/high): ";
            std::getline(std::cin, sec_level);
            if (sec_level.find("med") != -1)
            {
                config.security_level = RealSenseID::AuthConfig::SecurityLevel::Medium;
            }
            std::string rot_level;
            std::cout << "Set rotation level(0/180): ";
            std::getline(std::cin, rot_level);
            if (rot_level.find("180") != std::string::npos)
            {
                config.camera_rotation = RealSenseID::AuthConfig::CameraRotation::Rotation_180_Deg;
            }

            set_auth_settings_example(serial_config, config);
            break;
        }
        case 'p':
            pairing_example(serial_config);
            break;
        case 'b':
            standby_db_save(serial_config);
            break;
        case 'g':
            get_auth_settings_example(serial_config);
            break;
        case 'n':
            get_number_users_example(serial_config);
            break;
        case 'u':
            get_users_example(serial_config);
            break;
        case 'v':
            additional_information_example(serial_config);
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
            ping_example(serial_config, iters);
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
            enroll_faceprint_example(serial_config, user_id.c_str());
            break;
        }
        case 'A':
            authenticate_faceprint_example(serial_config);
            break;
        case 'U': {
            std::cout << std::endl << s_user_faceprint_db.size() << " users\n";
            for (const auto &iter:s_user_faceprint_db)
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
