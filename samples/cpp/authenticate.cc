// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/FaceAuthenticator.h"
#include <iostream>

class MyAuthClbk : public RealSenseID::AuthenticationCallback
{
public:
    void OnResult(const RealSenseID::AuthenticateStatus status, const char* user_id) override
    {
        if (status == RealSenseID::AuthenticateStatus::Success)
            std::cout << "Authenticated " << user_id << std::endl;
    }

    void OnHint(const RealSenseID::AuthenticateStatus hint) override
    {
        std::cout << "OnHint " << hint << std::endl;
    }

    void OnFaceDetected(const std::vector<RealSenseID::FaceRect>& faces) override
    {
        for (auto& face : faces)
        {
            std::cout << "Detected face " << face.x << "," << face.y << " " << face.w << "x" << face.h << std::endl;
        }
    }
};

int main()
{
    RealSenseID::FaceAuthenticator authenticator;
    auto status = authenticator.Connect({"COM9"});
    if (status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed connecting with status " << status << std::endl;
        return 1;
    }
    MyAuthClbk auth_clbk;
    authenticator.Authenticate(auth_clbk);
}