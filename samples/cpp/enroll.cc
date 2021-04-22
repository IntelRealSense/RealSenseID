// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/FaceAuthenticator.h"
#include <iostream>

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

int main()
{
    RealSenseID::FaceAuthenticator authenticator;
    auto status = authenticator.Connect({"COM9"});
    if (status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed connecting with status " << status << std::endl;
        return 1;
    }
    MyEnrollClbk enroll_clbk;
    authenticator.Enroll(enroll_clbk, "john");
}