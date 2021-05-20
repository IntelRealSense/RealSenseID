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
 * Android (tested on Android 6.0 but should also work on newer versions)

## Building
Use CMake version 3.10.2 or above:

```console
$ cd <project_dir>
$ mkdir build && cd build
$ cmake .. //in case preview is required run: cmake .. -DRSID_PREVIEW=1 //in case secure is required run: cmake .. -DRSID_SECURE=1
$ make -j
```

## Sample Code
This snippet shows the basic usage of our library.
```cpp
// Create face authenticator and connect it to the device on port COM9
RealSenseID::FaceAuthenticator authenticator;
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

```

For additional languages, build instruction and detailed code please see our code [samples](./samples) and [tools](tools).

## Secure Communication
The library can be compiled in secure mode.
Once secure communication is enabled, the communication with the device is encrypted using the Elliptic-curve Diffie–Hellman key exchange (ECDH).

To enable secure mode, the host should perform the following steps:
* Compile the library with `RSID_SECURE` defined. e.g. ```cmake -DRSID_SECURE=ON ..```
* Generate a set of ECDSA public and private keys. The host is responsible for keeping his private key safe.
* Pair with the device to enable secure communication. Pairing is performed once using the FaceAuthenticator API.
* Implement a [SignatureCallback](./include/RealSenseID/SignatureCallback.h). Signing and verifying is done with the ECDSA keys.
  Please see the [pairing sample](samples/cpp/pair-device.cc) on how to pair the device and use keys.

Each request (a call to one of the main API functions listed below) from the host to the device starts a new encrypted session, which performs an ECDH key exchange to create the shared secret for encrypting the current session.

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
char* hostPubKey = getHostPubKey(); // 64 bytes ECDSA public key of the host
char host_pubkey_signature[64];
if (!sig_clbk.Sign((unsigned char*)hostPubKey, 64, host_pubkey_signature))
{
    printf("Failed to sign host ECDSA key\n");
    return;
}
char device_pubkey[64]; // Save this key after getting it to use it for verification
Status pair_status = authenticator.Pair(host_pubkey, host_pubkey_signature, device_pubkey);
```

## Server and Device APIs
### Device Mode
Device Mode is a set of APIs enable the user to enroll and authenticate on the device itself, 
including database management and matching on the device.

### Server Mode
Server Mode is a set of APIs for users who wish to manage a faceprints database
on the host or the server. In this mode F450 is used as a Faceprints-Extraction device only.

The API provides a 'matching' function which predicts whether two faceprints belong to the same person,
thus enabling the user to scan their database for similar users.


## Project Structure
### RealSenseID API
The [RealSenseID](./include/RealSenseID/) is the main API to the communication with the device.
C, C++ and C# API wrappers are provided as part of the library.
Java and python API is under development.
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
Currently 704x1280 or 1056x1920 RGB formats is available.

##### Sensor Timestamps
Access to the sensor timestamps (in milliseconds) of each frame is available **on Windows only**.  
To enable it you will need:
1. WinSDK ver 10 (10.0.15063) or later. install from [windows-10-sdk download](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk/) .

2. Dedicated registry entry to be present for each unique RealSenseID device.
run the following script: [scripts/realsenseid_metadata_win10.ps1](./scripts/realsenseid_metadata_win10.ps1) using powershell (Admin privileges required).  
The script enables metadata for all realsenseID devices connected to the computer, permemntntly.

The timestamps can be acquired in OnPreviewImageReady under *image.metadata.timestamp* . Other metadata isn't valid.

more information about metadata on windows can be found in [microsoft uvc documetnation](https://docs.microsoft.com/en-us/windows-hardware/drivers/stream/uvc-extensions-1-5#2211-still-image-capture--method-2)

##### StartPreview
Starts preview. Callback function that is provided as parameter will be invoked for a newly arrived image and can be rendered by your application.
```cpp
class PreviewRender : public RealSenseID::PreviewImageReadyCallback
{
public:
	void OnPreviewImageReady(const Image image)
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

#### Server Mode Methods
##### ExtractFaceprintsForAuth
Extracts faceprints from a face, in the device, and sends them to the host.
Uses 'Authentication Flow' to eliminate spoof attempts and verify a face was detected.

```cpp
// extract faceprints using authentication flow
class FaceprintsAuthClbk : public RealSenseID::AuthFaceprintsExtractionCallback
{
public:
    void OnResult(const RealSenseID::AuthenticateStatus status, const Faceprints* faceprints) override
    {
        std::cout << "result: " << status << std::endl;
        s_extraction_status = status;
        // if status was success pass '_faceprints' to user 
    }

    void OnHint(const RealSenseID::AuthenticateStatus hint) override
    {
        std::cout << "hint: " << hint << std::endl;
    }
};

FaceprintsAuthClbk clbk;
auto status = authenticator->ExtractFaceprintsForAuth(clbk);
```

##### ExtractFaceprintsForEnroll
Extracts faceprints from a face, in the device, and sends them to the host.
Uses 'Enrollment Flow' to verify face-pose is correct and that a face was detected.

```cpp
// extract faceprints using enrollment flow
class MyEnrollServerClbk : public RealSenseID::EnrollmentCallback
{
    const char* _user_id = nullptr;

public:
    explicit MyEnrollServerClbk(const char* user_id) : _user_id {user_id}
    {        
    }

    void OnResult(const RealSenseID::EnrollStatus status, const Faceprints* faceprints) override
    {
        std::cout << "result: " << status << "for user: " << _user_id << std::endl;
        s_extraction_status = status;    
        // if status was success pass '_faceprints' to user            
    }

    void OnProgress(const RealSenseID::FacePose pose) override
    {
        std::cout << "pose: " << pose << std::endl;
    }

    void OnHint(const RealSenseID::EnrollStatus hint) override
    {
        std::cout << "hint: " << hint << std::endl;
    }
};

MyEnrollServerClbk enroll_clbk {user_id.c_str()};
auto status = authenticator->ExtractFaceprintsForEnroll(enroll_clbk);
```

##### ExtractFaceprintsForAuthLoop
Extracts faceprints in a loop, each iteration extracts from a single face and sends it to the host.
Uses 'Authentication Flow' to eliminate spoof attempts and verify a face was detected.

```cpp
// extract faceprints in a loop using authentication flow
class AuthLoopExtrClbk : public RealSenseID::AuthFaceprintsExtractionCallback
{    
public:
    void OnResult(const RealSenseID::AuthenticateStatus status, const Faceprints* faceprints) override
    {        
        std::cout << "result: " << status << std::endl;
        s_extraction_status = status;
        // if status was success pass '_faceprints' to user 
    }

    void OnHint(const RealSenseID::AuthenticateStatus hint) override
    {
        std::cout << "hint: " << hint << std::endl;
    }
};

AuthLoopExtrClbk clbk;
auto status = authenticator->ExtractFaceprintsForAuthLoop(clbk);
```

##### MatchFaceprints
Matches two faceprints to each other and calculates a prediction of whether they belong to
the same person.

```cpp
// match extracted faceprints to the ones in the user-managed database
for (auto& db_item : db)
{
    auto match_result = authenticator->MatchFaceprints(scanned_faceprints, db_item.faceprints, updated_faceprints);    
    if (match_result.success)
    {
        std::cout << "Match succeeded with user id: " << db_item.id << std::endl;
        break;
    }
}
```

### PacketManager
The [PacketManager](./src/PacketManager/) is the communication protocol implementation.
Communication is encrypted and managed by [PacketManager::SecureSessionManager](./src/PacketManager/SecureHostSession.h).

#### SecureSessionManager
Manages the secure session. Sends packets using [PacketManager::PacketSender](./src/PacketManager/PacketSender.h).

#### PacketSender
Responsible for sending and receiving serial packets over a serial port using [PacketManager::SerialConnection](./src/PacketManager/SerialConnection.h) interface.
#### SerialConnection
Interface for communication over a serial port. Implemented over supported platforms ([WindowsSerial](./src/PacketManager/WindowsSerial.h)/[LinuxSerial](./src/PacketManager/LinuxSerial.h)/[Android](./src/PacketManager/AndroidSerial.h)).
**For new platform, implement this interface.**

#### SerialPacket
Packet structure base. We have 2 types of packets:
* DataPacket - Payload contains data buffer.
* FaPacket - Payload contains user id and status.

## License
This project is licensed under Apache 2.0 license. Relevant license info can be found in "License Notices" folder.

## Intel RealSense ID F450 and F455 Architecure Diagram
![plot](./docs/F450_Architecture.png?raw=true)
