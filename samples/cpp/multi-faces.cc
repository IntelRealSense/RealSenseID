// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

// Sample of how to correlate detected face rectangle with authenticated user

#include "RealSenseID/FaceAuthenticator.h"
#include "RealSenseID/Preview.h"
#include <iostream>
#include <vector>

RealSenseID::DeviceConfig device_config;

class PreviewRender : public RealSenseID::PreviewImageReadyCallback
{
    std::vector<unsigned int> _images_ts;

public:
    void OnPreviewImageReady(const RealSenseID::Image image)
    {
        std::cout << image.metadata.timestamp << std::endl;
        _images_ts.push_back(image.metadata.timestamp); // save timestamps for matching to onFaceDetected
        // image can be saved to file with timestamp in name
    }

    const std::vector<unsigned int>& GetImagesTimeStamps()
    {
        return _images_ts;
    }
};

class MyAuthClbk : public RealSenseID::AuthenticationCallback
{
    std::vector<RealSenseID::FaceRect> _faces;
    size_t _result_idx = 0;
    unsigned int _ts =0;

public:
    void OnResult(const RealSenseID::AuthenticateStatus status, const char* user_id) override
    {        
        std::cout << "\n******* OnResult #" << _result_idx << "*******" << std::endl;
        std::cout << "Status: " << status << std::endl;
        if (status == RealSenseID::AuthenticateStatus::Success)
        {
            if (device_config.algo_flow == RealSenseID::DeviceConfig::AlgoFlow::SpoofOnly)
            {
                std::cout << " Real face" << std::endl;
            }
            else
            {
                std::cout << "UserId: " << user_id << std::endl;
            }
        }
        else if (status == RealSenseID::AuthenticateStatus::Forbidden)
        {
            std::cout << " User is not authenticated" << std::endl;
        }
        else if (status == RealSenseID::AuthenticateStatus::Spoof)
        {
            std::cout << " Spoof" << std::endl;
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
        _ts = ts;
    }

    unsigned int GetLastTimeStamp()
    {
        return _ts;
    }
};

int main()
{
    RealSenseID::FaceAuthenticator authenticator;
    RealSenseID::PreviewConfig preview_config;
    RealSenseID::Preview preview(preview_config);

#ifdef _WIN32
    auto status = authenticator.Connect({"COM25"});
#elif LINUX
    auto status = authenticator.Connect({"/dev/ttyACM0"});
#endif
    if (status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed connecting with status " << status << std::endl;
        return 1;
    }


    // uncomment below line to configure FW to run Spoof detection only, by default all algo(s) are running
    // device_config.algo_flow = RealSenseID::DeviceConfig::AlgoFlow::SpoofOnly;

    // configure FW to authenticate or detect spooofs all detected faces
    device_config.face_selection_policy = RealSenseID::DeviceConfig::FaceSelectionPolicy::All;
    
    // apply configuration
    status = authenticator.SetDeviceConfig(device_config);

    PreviewRender image_clbk;
    MyAuthClbk auth_clbk;

    preview.StartPreview(image_clbk);
    authenticator.Authenticate(auth_clbk);
    preview.StopPreview();

    // get timestamps saved in callbacks
    auto face_detection_ts = auth_clbk.GetLastTimeStamp();
    auto images_ts = image_clbk.GetImagesTimeStamps();

    if (face_detection_ts != 0)
    {
        // looking for the preview image that is closest to face detection by timestamp
        for (auto it = images_ts.begin(); it != images_ts.end(); ++it)
        {
            if (*it >= face_detection_ts)
            {
                auto most_close =
                    (*it - face_detection_ts < face_detection_ts - *(it -1)) ? it : (it-1); // chose closest

                std::cout << "The image matched to OnFaceDetected is the #" << most_close - images_ts.begin()
                          << " image accquired." << std::endl;
                std::cout << "Face detection timestamp: " << face_detection_ts << " milliseconds." << std::endl;
                std::cout << "Image timestamp: " << *most_close << " milliseconds." << std::endl;
                break;
            }
        }
    }
}
