// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/FaceAuthenticator.h"
#include "RealSenseID/Preview.h"
#include "RealSenseID/DeviceController.h"
#include "RealSenseID/SignatureCallback.h"
#include "rsid_signature_example.h"

#include <string>
#include <stdexcept>
#include <iostream>
#include <cstdlib>

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

void enroll_example(RealSenseID::FaceAuthenticator& authenticator, const char* user_id)
{
    MyEnrollClbk enroll_clbk;
    auto enroll_status = authenticator.Enroll(enroll_clbk, user_id);
    std::cout << "Final status:" << enroll_status << std::endl << std::endl;
}

void set_auth_settings_example(RealSenseID::FaceAuthenticator& authenticator, RealSenseID::AuthConfig& auth_config)
{
    std::cout << "Set authentication settings example ..." << std::endl;
    auto status = authenticator.SetAuthSettings(auth_config);
    std::cout << "Final status:" << status << std::endl << std::endl;
}

//
// Authentication example
//
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

void authenticate_example(RealSenseID::FaceAuthenticator& authenticator)
{
    MyAuthClbk auth_clbk;
    auto auth_status = authenticator.Authenticate(auth_clbk);
    std::cout << "Final status:" << auth_status << std::endl << std::endl;
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
    if (ser_type == "usb")
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

void print_menu()
{
    std::cout << "Enter a command:\n";
    std::cout << "  'a' to authenticate.\n  'e' to enroll.\n  'd' to delete all users.\n  's' to set security level.\n "
                 " 'q' to quit."
              << std::endl;
}

void sample_loop(RealSenseID::FaceAuthenticator& authenticator)
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
            enroll_example(authenticator, user_id.c_str());
            break;
        }
        case 'a':
            authenticate_example(authenticator);
            break;

        case 'd':
            authenticator.RemoveAll();
            break;
        case 's': {
            RealSenseID::AuthConfig config;
            config.camera_rotation = RealSenseID::AuthConfig::Rotation_90_Deg;
            config.security_level = RealSenseID::AuthConfig::High;
            std::string sec_level;
            std::cout << "Set security level(medium/high): ";
            std::getline(std::cin, sec_level);
            if (sec_level.find("med") != -1)
            {
                config.security_level = RealSenseID::AuthConfig::Medium;
            }
            std::string rot_level;
            std::cout << "Set rotation level(90/180): ";
            std::getline(std::cin, rot_level);
            if (rot_level.find("180") != -1)
            {
                config.camera_rotation = RealSenseID::AuthConfig::Rotation_180_Deg;
            }

            set_auth_settings_example(authenticator, config);
            break;
        }
        case 'q':
            is_running = false;
            break;
        }
    }
}


int main(int argc, char* argv[])
{
    auto config = config_from_argv(argc, argv);
    RealSenseID::Examples::SignClbk sig_clbk;
    RealSenseID::FaceAuthenticator auth {&sig_clbk};

    auto connect_status = auth.Connect(config);
    if (connect_status != RealSenseID::SerialStatus::Ok)
    {
        std::cout << "Failed connecting to port " << config.port << " status:" << connect_status << std::endl
                  << std::endl;
        return 1;
    }

    sample_loop(auth);
    return 0;
}
