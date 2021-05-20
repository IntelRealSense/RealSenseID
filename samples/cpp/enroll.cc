// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/FaceAuthenticator.h"
#include <iostream>
#include <unistd.h>
#include <string.h>

void showHelp(char *_run)
{     
      std::cout<<"Usage : " << _run << " -u [UserID]" << std::endl;
}

class MyEnrollClbk : public RealSenseID::EnrollmentCallback
{
public:
    void OnResult(const RealSenseID::EnrollStatus status) override
    {
        std::cout << "Result " << status << std::endl;
    }

    void OnProgress(const RealSenseID::FacePose pose) override
    {
        std::cout << "OnProgress " << pose << std::endl;
    }

    void OnHint(const RealSenseID::EnrollStatus hint) override
    {
        std::cout << "Hint " << hint << std::endl;
    }

    void OnFaceDetected(const std::vector<RealSenseID::FaceRect>& faces) override
    {
        for (auto& face : faces)
        {
            std::cout << "Detected face " << face.x << "," << face.y << ", " << face.w << "x" << face.h << std::endl;
        }
    }
};

int main(int argc, char **argv)
{
    if(argc<2)
    {
        showHelp(argv[0]);
        exit(0);
    }
    int c;
    const char *user_id = NULL;
    RealSenseID::FaceAuthenticator authenticator;
#ifdef _WIN32
    auto status = authenticator.Connect({"COM9"});
#elif LINUX
    auto status = authenticator.Connect({"/dev/ttyACM0"});
#endif
    while ((c = getopt(argc, argv, "u:")) != -1)
    {
        switch(c)
        {
            case 'u':
            optind--;
            user_id = strdup(argv[optind]);
            break;
        }
    }

    if(status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed connecting with status " << status << std::endl;
        return 1;
    }
    MyEnrollClbk enroll_clbk;
    authenticator.Enroll(enroll_clbk, user_id);
}
