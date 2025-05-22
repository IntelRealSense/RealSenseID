// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/Preview.h"
#include "RealSenseID/FaceAuthenticator.h"
#include <chrono>
#include <thread>
#include <iostream>

#ifdef _WIN32
static const char* CONNECTION_STRING = "COM72";
#elif LINUX
static const char* CONNECTION_STRING = "/dev/ttyACM0";
#endif

class EmptyAuthenticationCallback : public RealSenseID::AuthenticationCallback
{
public:
    void OnResult(const RealSenseID::AuthenticateStatus status, const char* user_id) override
    {
    }

    void OnHint(const RealSenseID::AuthenticateStatus hint) override
    {
    }

    void OnFaceDetected(const std::vector<RealSenseID::FaceRect>& faces, const unsigned int ts) override
    {
    }
};

class PreviewSampleCallback : public RealSenseID::PreviewImageReadyCallback
{
public:
    void OnPreviewImageReady(const RealSenseID::Image& image)
    {
        std::cout << "preview -> " << image.width << "x" << image.height << " (" << image.size << "B)\n";
    }

    void OnSnapshotImageReady(const RealSenseID::Image image)
    {
        std::cout << "snapshot -> " << image.width << "x" << image.height << " (" << image.size << "B)\n";
    }
};

int main()
{
    RealSenseID::PreviewConfig preview_config;
    RealSenseID::Preview preview(preview_config);
    PreviewSampleCallback preview_callback;

    auto device_type = RealSenseID::DeviceType::F45x; // or F46x for f46x devices, or use DiscoverDeviceType(..) if not sure
    RealSenseID::FaceAuthenticator authenticator(device_type);
    auto status = authenticator.Connect({CONNECTION_STRING});
    if (status != RealSenseID::Status::Ok)
    {
        std::cout << "connection failed!\n";
        return 1;
    }

    RealSenseID::DeviceConfig device_config;
    device_config.dump_mode = RealSenseID::DeviceConfig::DumpMode::CroppedFace;

    status = authenticator.SetDeviceConfig(device_config);
    if (status != RealSenseID::Status::Ok)
    {
        std::cout << "failed to apply device settings!\n";

        authenticator.Disconnect();
        return 1;
    }

    EmptyAuthenticationCallback authentication_callback;
    authenticator.Authenticate(authentication_callback);

    std::cout << "starting preview...\n";

    preview.StartPreview(preview_callback);
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "sending authentication request, snapshots should arrive if a face is detected\n";
    authenticator.Authenticate(authentication_callback);

    std::this_thread::sleep_for(std::chrono::seconds(3));

    preview.StopPreview();
}
