# Intel® RealSense™ ID Solution for Facial Authentication

## Overview
Intel RealSense ID is your trusted facial authentication on-device solution.

Intel RealSense ID combines an active depth sensor with a specialized neural network designed to deliver an intuitive, secure and accurate facial authentication solution that adapts over time.
This solution offers user privacy and is activated by user awareness. Built-in anti spoofing technology protects against attacks using photographs, videos or masks.

Intel RealSense ID is a natural solution simplifying secure entry for everyone, everywhere. Supports children to tall adults and designed for Smart Locks, Access Control, PoS, ATMs, and Kiosks.

Developer kits containing the necessary hardware to use this library are available for purchase at store.intelrealsense.com. 
See information about the Intel RealSense ID technology at https://www.intelrealsense.com/facial-authentication/

For high-level architecture, see [Intel RealSense ID F450 / F455 Architecture Diagram](#Intel-RealSense-ID-F450-and-F455-Architecure-Diagram).

Note: Device = Intel RealSense ID F450 / F455

## Platforms
 * Linux (tested on ubuntu 18, gcc 7.5+)
 * Windows (tested on windows 10, msvc 2019)

## Building
Use CMake version 3.13 or above:
```console
$ cd <project_dir>
$ mkdir build && cd build
$ cmake .. // in case preview is required run: cmake .. -D RSID_PREVIEW=1
$ make -j
```

## Sample Code
This snippet shows the basic usage of our library.
```cpp
// Start preview
PreviewRender image_clbk;
Preview preview;
bool success = preview.StartPreview(image_clbk);

// Create device controller instance and connect to the device on COM9 which is of type USB
RealSenseID::DeviceController deviceController;
auto connect_status = deviceController.Connect({RealSenseID::SerialType::USB, "COM9"});

// Pair with host
auto pair_status = deviceController.Pair(host_pubkey, host_pubkey_signature, device_pubkey);

// Disconnect from the device
deviceController.Disconnect();

// Create face authenticator instance and connect to the device on COM9 which is of type USB
MySigClbk sig_clbk;
RealSenseID::FaceAuthenticator authenticator {&sig_clbk};
connect_status = authenticator.Connect({RealSenseID::SerialType::USB, "COM9"});

// Enroll a user
const char* user_id = "John";
MyEnrollClbk enroll_clbk;
auto status = authenticator.Enroll(enroll_clbk, user_id);

// Authenticate a user
MyAuthClbk auth_clbk;
status = authenticator.Authenticate(auth_clbk);

// Remove the user from device
success = authenticator.RemoveUser(user_id);

// Disconnect from the device
authenticator.Disconnect();

// Stop preview
success = preview.StopPreview();
```

For additional languages, build instruction and detailed code please see our [examples](./examples).

## Secure Communication
The communication with the device is encrypted using the Elliptic-curve  Diffie–Hellman key exchange (ECDH).

To enable secure communication, the host should perform the following steps:
* Generate a set of ECDSA public and private keys. The host is responsible for keeping his private key safe.
* Pair with the device to enable secure communication. Pairing is performed once using the FaceAuthenticator API.
* Implement a [SignatureCallback](./include/RealSenseID/SignatureCallback.h). Signing and verifying is done with the ECDSA keys.
  Please see the [signature callback example](examples/shared/signature_example/rsid_signature_example.cc).

Each request (a call to one of the main API functions listed below) from the host to the device starts a new encrypted session, which performs an ECDH key exchange to create the shared secret for encrypting the current session.

## Project Structure
### <ins>RealSenseID API</ins>
The [RealSenseID](./include/RealSenseID/) is the main API to the communication with the device.</br>
C, C++ and C# API wrappers are provided as part of the library.</br>
Java and python API is under development.</br>
All APIs are synchronous and assuming a new operation will not be activated till previous is completed.

* C++ [API](./include/RealSenseID)
* C/C# [API](./wrappers)

#### FaceAuthenticator
##### Connect / Disconnect
Connects host to device using USB (```RealSenseID::SerialType::USB```) or UART interfaces (```RealSenseID::SerialType::UART```).
```cpp
class MySigClbk : public RealSenseID::SignatureCallback
{
public:
    bool Sign(const unsigned char* buffer, const unsigned int buffer_len, unsigned char* out_sig) override
    {
      // Sign buffer with host ECDSA private key
    }

    bool Verify(const unsigned char* buffer, const unsigned int buffer_len, const unsigned char* sig,const unsigned int sign_len) override
    {
      // Verify buffer with device ECDSA public key
    }
};

MySigClbk sig_clbk;
RealSenseID::FaceAuthenticator authenticator {&sig_clbk};
Status connect_status = authenticator.Connect({RealSenseID::SerialType::USB, "COM9"});
authenticator.Disconnect();
```

##### Pair
Perform an ECDSA key exchange.
In pairing, the host sends the following:
* ECDSA public key.
* ECDSA public key signature with the previous ECDSA private key. If this is the first pairing, the signature is ignored.
```cpp
Status pair_status = authenticator.Pair(host_pubkey, host_pubkey_signature, device_pubkey);
```

The Host will receive the device's ECDSA public key, and should save it for verification in the implementation of SignatureCallback::Verify.

##### Enrollment
Starts device, runs neural network algorithm and stores encrypted faceprints on database.
A faceprint is a set number of points which is represented as mathematical transformation of the user’s face. saves encrypted facial features to secured flash on Intel RealSense ID F450 / F455.
Stored encrypted faceprints are matched with enrolled faceprints later during authentication.
For best performance, enroll under normal lighting conditions and look directly at the device.
During the enrollment process, device will send a status *hint* to the callback provided by application.
Full list of the *hint* can be found in [EnrollStatus.h](./include/RealSenseID/).
```cpp
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

const char* user_id = "John";
MyEnrollClbk enroll_clbk;
Status status = authenticator.Enroll(enroll_clbk, user_id);
```

##### Authenticate
Single authentication attempt: Starts device, runs neural network algorithm, generates faceprints and compares them to all enrolled faceprints in database.
Finally, returns whether the authentication was forbidden or allowed with enrolled user id. During the authentication process, device will send a status *hint* to the callback provided by application.
Full list of the *hint* can be found in [AuthenticationStatus.h](./include/RealSenseID/).
```cpp
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

MyAuthClbk auth_clbk;
Status status = authenticator.Authenticate(auth_clbk);
```

##### AuthenticateLoop
Starts device, runs authentication in a loop until Cancel() API is invoked.
Starts camera and running loop of
- NN algo(s) pipeline
- Facial features extraction
- Matches them to all previously stored features in database.
- Returns result whether the authentication was forbidden or allowed with pre-enrolled user id.
During the authentication process hint statuses might be sent to the callback provided by application.
Full list of the hints can be found in [AuthenticationStatus.h](./include/RealSenseID/).
```cpp
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

MyAuthClbk auth_clbk;
Status status = authenticator.AuthenticateLoop(auth_clbk);
```

##### Cancel
Stops current operation (Enrollment/Authenticate/AuthenticateLoop).
```cpp
authenticator.Cancel();
```

##### RemoveAll
Removes all enrolled users in the device database.
```cpp
bool success = authenticator.RemoveAll();
```

##### RemoveUser
Removes a specific user from the device database.
```cpp
const char* user_id = "John";
bool success = authenticator.RemoveUser(user_id);
```

#### DeviceController
##### Connect / Disconnect
Connects host to device using USB (```RealSenseID::SerialType::USB```) or UART interfaces (```RealSenseID::SerialType::UART```).
```cpp
RealSenseID::DeviceController deviceController;
Status connect_status = deviceController.Connect({RealSenseID::SerialType::USB, "COM9"});
deviceController.Disconnect();
```

##### Reboot
Resets and reboots device.
```cpp
bool success = deviceController.Reboot();
```

##### FW Upgrade (Not supported yet)


#### Preview API
Host needs to have OpenCV installed.
Currently 640x352 YUV format is available.

##### StartPreview
Starts preview. Callback function that is provided as parameter will be invoked for a newly arrived image and can be rendered by your application.
```cpp
class PreviewRender : public RealSenseID::PreviewImageReadyCallback
{
public:
	void OnPreviewImageReady(const Image image){
	{
		// render image
	}
};

PreviewRender image_clbk;
Preview preview;
bool success = preview.StartPreview(image_clbk);
```

##### PausePreview
Pause preview.
```cpp
bool success = preview.PausePreview();
```

##### ResumePreview
Resumes the preview.
```cpp
bool success = preview.ResumePreview();
```

##### StopPreview
Stops preview.
```cpp
bool success = preview.StopPreview();
```

### <ins>PacketManager</ins>
The [PacketManager](./src/PacketManager/) is the communication protocol implementation.</br>
Communication is encrypted and managed by [PacketManager::SecureSessionManager](./src/PacketManager/SecureHostSession.h).

#### SecureSessionManager
Manages the secure session. Sends packets using [PacketManager::PacketSender](./src/PacketManager/PacketSender.h).

#### PacketSender
Responsible for sending and receiving serial packets over a serial port using [PacketManager::SerialConnection](./src/PacketManager/SerialConnection.h) interface.
#### SerialConnection
Interface for communication over a serial port. Implemented over supported platforms ([WindowsSerial](./src/PacketManager/WindowsSerial.h)/[LinuxSerial](./src/PacketManager/LinuxSerial.h)).</br>
**For new platform, implement this interface.**

#### SerialPacket
Packet structure base. We have 2 types of packets:
* DataPacket - Paybload contains data buffer.
* FaPacket - Paybload contains user id and status.

## License
This project is licensed under Apache 2.0 license. Relevant license info can be found in "License Notices" folder.

## Intel RealSense ID F450 and F455 Architecure Diagram
![plot](./docs/F450_Architecture.png?raw=true)
