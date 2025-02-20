// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.


// Example on how to pair the device with the host,
// Pairing is needed to enable secure communication with the device.
// NOTE: you must use your own private/public key in production instead of the one in the example.
#include "RealSenseID/FaceAuthenticator.h"
#include "secure_mode_helper.h"
#include <iostream>

void pair_device(RealSenseID::FaceAuthenticator& authenticator, RealSenseID::Samples::SignHelper& signer)
{
    char* host_pubkey = (char*)signer.GetHostPubKey();
    char host_pubkey_signature[32] = {0};
    char device_pubkey[64] = {0};
    auto pair_status = authenticator.Pair(host_pubkey, host_pubkey_signature, device_pubkey);
    if (pair_status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed pairing with device" << std::endl;
        return;
    }
    signer.UpdateDevicePubKey((unsigned char*)device_pubkey);
    std::cout << "Final status:" << pair_status << std::endl << std::endl;
}

int main()
{
    RealSenseID::Samples::SignHelper secure_helper;
    RealSenseID::FaceAuthenticator authenticator {&secure_helper};
#ifdef _WIN32
    authenticator.Connect({"COM9"});
#elif LINUX
    authenticator.Connect({"/dev/ttyACM0"});
#endif
    pair_device(authenticator, secure_helper);
}
