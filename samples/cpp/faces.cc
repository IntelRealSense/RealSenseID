// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

// Sample of how to correlate detected face rectangle with authenticated user

#include "RealSenseID/FaceAuthenticator.h"
#include <iostream>
#include <vector>

class MyAuthClbk : public RealSenseID::AuthenticationCallback
{
    std::vector<RealSenseID::FaceRect> _faces;
    size_t _result_idx = 0;

public:
    void OnResult(const RealSenseID::AuthenticateStatus status, const char* user_id) override
    {        
        std::cout << "\n******* OnResult #" << _result_idx << "*******" << std::endl;
        std::cout << "Status: " << status << std::endl;
        if (status == RealSenseID::AuthenticateStatus::Success)
        {
            std::cout << "UserId: " << user_id << std::endl;
        }

        // print the corresponding face coords (that was received in OnFaceDetected)
        if (_faces.size() > _result_idx)
        {
            auto& face = _faces[_result_idx];
            std::cout << "Face: " << face.x << "," << face.y << " " << face.w << "x" << face.h << std::endl;
        }
        _result_idx++;
        std::cout << std::endl;
    }

    void OnHint(const RealSenseID::AuthenticateStatus hint) override
    {
        std::cout << "OnHint " << hint << std::endl;
    }

    void OnFaceDetected(const std::vector<RealSenseID::FaceRect>& faces, const unsigned int ts) override
    {
        _faces = faces;
    }
};

int main()
{
    RealSenseID::FaceAuthenticator authenticator;
#ifdef _WIN32
    auto status = authenticator.Connect({"COM9"});
#elif LINUX
    auto status = authenticator.Connect({"/dev/ttyACM0"});
#endif
    if (status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed connecting with status " << status << std::endl;
        return 1;
    }
    MyAuthClbk auth_clbk;
    authenticator.Authenticate(auth_clbk);
}
