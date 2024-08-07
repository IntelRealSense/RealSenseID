# Intel® RealSense™ ID Solution for Facial Authentication

## Table of Contents
1. [Overview](#overview)
2. [Platforms](#platforms)
3. [Building](#building)
4. [Sample Code](#sample-code)
5. [Secure Communication](#secure-communication)
6. [Server and Device APIs](#server-and-device-apis)
7. [Project Structure](#project-structure)
8. [Setting Product Key](#product-keys)
9. [License](#license)
9. [Intel RealSense ID F450 and F455 Architecure Diagram](#intel-realsense-id-f450-and-f455-architecure-diagram)

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
 * Linux (tested on Ubuntu 18, gcc 7.5+)
 * Windows (tested on Windows 10, msvc 2019) 

## Building
Use CMake version 3.14 or above:

```console
$ cd <project_dir>
$ mkdir build && cd build
$ cmake .. //in case preview is required run: cmake .. -DRSID_PREVIEW=1 //in case secure is required run: cmake .. -DRSID_SECURE=1
$ make -j
```

### CMake Options

The following possible options are available for the `cmake` command

| Option               | Default | Feature                                          |
|----------------------|:-------:|:-------------------------------------------------|
| `RSID_DEBUG_CONSOLE` |  `ON`   | Log everything to console                        |
| `RSID_DEBUG_FILE`    |  `OFF`  | Log everything to _rsid_debug.log_ file          |
| `RSID_DEBUG_SERIAL`  |  `OFF`  | Log all serial communication                     |
| `RSID_DEBUG_PACKETS` |  `OFF`  | Log packet sent/received over the serial line    |
| `RSID_DEBUG_VALUES`  |  `OFF`  | Replace default common values with debug ones    |
| `RSID_PREVIEW`       |  `OFF`  | Enables preview feature.                         |
| `RSID_SAMPLES`       |  `OFF`  | Build samples                                    |
| `RSID_TIDY`          |  `OFF`  | Enable clang-tidy                                |
| `RSID_PEDANTIC`      |  `OFF`  | Enable extra compiler warnings                   |
| `RSID_PROTECT_STACK` |  `OFF`  | Enable stack protection compiler flags           |
| `RSID_DOXYGEN`       |  `OFF`  | Build doxygen docs                               |
| `RSID_SECURE`        |  `OFF`  | Enable secure communication with device          |
| `RSID_TOOLS`         |  `ON`   | Build additional tools                           |
| `RSID_PY`            |  `OFF`  | Build python wrapper                             |
| `RSID_INSTALL`       |  `OFF`  | Generate the install target and rsidConfig.cmake |

### Linux Post Install

In order to install the udev rules for the F450/F455 devices, run the command:
```bash
sudo script/udev-setup.sh -i
```
Note: You can undo the changes/uninstall by using the `-u` flag. 

The capture device will be available to `plugdev` users while the serial port will be available to `dialout` users.
On Debian/Ubuntu, you can add current user to those groups by issuing the commands:
```bash
sudo adduser $USER dialout
sudo adduser $USER plugdev
```

Note: Changes in group membership will take effect after logout/login (or reboot). 

### Windows Post Install

In order to be able to capture metadata for RAW format, open a `PowerShell` terminal as Administrator and run the command
```shell
.\scripts\realsenseid_metadata_win10.ps1
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
The library can be compiled in secure mode. Once paired with the device, all communications will be protected.
If you wish to get back to non-secure communcations, you must first unpair your device.

Cryptographic algorithms that are used for session protection:
* Message signing - ECDSA P-256 (secp256-r1).
* Shared session key generation - ECDH P-256 (secp256-r1)
* Message encryption - AES-256, CTR mode.
* Message integrity - HKDF-256.

To enable secure mode, the host should perform the following steps:
* Compile the library with RSID_SECURE enabled.
* Generate a set of ECDSA P-256 (secp256-r1) public and private keys. The host is responsible for keeping his private key safe.
* Pair with the device to enable secure communication. Pairing is performed once using the FaceAuthenticator API.
* Implement a SignatureCallback. Signing and verifying is done with the ECDSA P-256 (secp256-r1) keys. Please see the pairing sample on how to pair the device and use keys.

Each request (a call to one of the main API functions listed below) from the host to the device starts a new encrypted session, which performs an ECDH P-256 (secp256-r1) key exchange to create the shared secret for encrypting the current session.

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

* C++ [API](./include/RealSenseID) and [samples](./samples/cpp).
* C/C#/Python [API](./wrappers) and [samples](./samples).


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


This operation can be further configured by passing [DeviceConfig](include/RealSenseID/DeviceConfig.h) struct to the ```FaceAuthenticator::SetDeviceConfig(const DeviceConfig&)``` function.
See the [Device Configuration](#device-configuration-api) below for details.
```cpp
class MyAuthClbk : public RealSenseID::AuthenticationCallback
{
public:
    // Called when authentication result is available.
    // If there are multiple faces it will be called for each face detected.
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
    // Called when result is available for a detected face.
    // If the status==AuthenticateStatus::Success then the user_id will point to c string of the authenenticated user id.
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

   
    // Called when faces are detected. Can be single or multiple faces.
    // The faces vector contains coord X(,y,h,w)of faces detected 
    // Coords are in full HD resolution (1920x1080):
    //   x,y: top left face rect
    //   w,h: width/height of the face rect
    //   Note: The `X` coords are flipped which means X==0 is most right, and X==1920 is most left.         
    // The timestamp argument is the timestamp in millis of the frame that the faces were found in.    
    void OnFaceDetected(const std::vector<RealSenseID::FaceRect>& faces, const unsigned int timestamp) override
    {
        _faces = faces;
    }
};

MyAuthClbk auth_clbk;
Status status = authenticator.AuthenticateLoop(auth_clbk);
```

##### Device Configuration API
The device operation can be configured by passing the [DeviceConfig](include/RealSenseID/DeviceConfig.h) struct to the ```FaceAuthenticator::SetDeviceConfig(const DeviceConfig&)``` function.

The various options and default values are described below:
```cpp
struct RSID_API DeviceConfig
{
    /**
     * @enum CameraRotation
     * @brief Camera rotation.
     */
    enum class CameraRotation
    {
        Rotation_0_Deg = 0, // default
        Rotation_180_Deg = 1,
        Rotation_90_deg = 2,
        Rotation_270_deg = 3
    };

    /**
     * @enum SecurityLevel
     * @brief SecurityLevel to allow
     */
    enum class SecurityLevel
    {
        High = 0,   // high security, no mask support, all AS algo(s) will be activated
        Medium = 1, // default mode to support masks, only main AS algo will be activated.
    };
  
    /**
     * @enum AlgoFlow
     * @brief Algorithms which will be used during authentication
     */
    enum class AlgoFlow
    {
        All = 0,               // default
        FaceDetectionOnly = 1, // face detection only
        SpoofOnly = 2,         // spoof only
        RecognitionOnly = 3    // recognition only
    };

    /**
     * @enum FaceSelectionPolicy
     * @brief To run authentication on all (up to 5) detected faces vs single (closest) face
     */
    enum class FaceSelectionPolicy
    {
        Single = 0, // default, run authentication on closest face
        All = 1     // run authentication on all (up to 5) detected faces
    };

    enum class DumpMode
    {
        None = 0,
        CroppedFace = 1,
        FullFrame = 2,
    };

    CameraRotation camera_rotation = CameraRotation::Rotation_0_Deg;
    SecurityLevel security_level = SecurityLevel::Medium;
    AlgoFlow algo_flow = AlgoFlow::All;
    FaceSelectionPolicy face_selection_policy = FaceSelectionPolicy::Single;
    DumpMode dump_mode = DumpMode::None;
};
```

Notes: 
  * If ```SetDeviceConfig()``` never called, the device will use the default values described above.
  * ```SetDeviceConfig()``` can be called once. The settings will take effect for all future authentication sessions (until the device is restarted).
  * ```CameraRotation``` enable the algorithm to work with a rotated device. For preview rotation to match, you'll need to define previewConfig.portraitMode accordingly (see Preview section).

The following example configures the device to only detect spoofs (instead of the default full authentication):
```cpp
...
using namespace RealSenseID;
// config with default options except for the algo flow which is set to spoof only 
DeviceConfig device_config; 
device_config.algo_flow = AlgoFlow::SpoofOnly;
auto status = authenticator->SetDeviceConfig(device_config);

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

##### Preview Configuration
```cpp
/**
 * Preview modes
 */
enum class PreviewMode
{
    MJPEG_1080P = 0, // default
    MJPEG_720P = 1,
    RAW10_1080P = 2,
};

/**
 * Preview configuration
 */
struct RSID_API PreviewConfig
{
    int cameraNumber = -1; // attempt to auto detect by default
    PreviewMode previewMode = PreviewMode::MJPEG_1080P; // RAW10 requires custom fw support
    bool portraitMode = true; 
};
```
Notes:
* The rotation used by algorithm is based only on DeviceConfig.camera_rotation attribute.
* Indepedently, you can choose each preview mode (except raw) to be portrait or non-portrait. 
* Keep in mind that if you want preview to match algo:  
CameraRotation::Rotation_0_Deg and CameraRotation::Rotation_180_Deg is for portraitMode == true.(default)  
CameraRotation::Rotation_90_Deg and CameraRotation::Rotation_270_Deg is for portraitMode == false.

##### Sensor Timestamps
Access to the sensor timestamps (in milliseconds).  
To enable it on Windows please turn on the Metadata option in when using the SDK installer.
The installer create specific dedicated registry entry to be present for each unique RealSenseID device.
For Linux, Metadata is supported on kernels 4.16 + only.

The timestamps can be acquired in OnPreviewImageReady under *image.metadata.timestamp* . Other metadata isn't valid.

more information about metadata on Windows can be found in [microsoft uvc documetnation](https://docs.microsoft.com/en-us/windows-hardware/drivers/stream/uvc-extensions-1-5#2211-still-image-capture--method-2)

##### StartPreview
Starts preview. Callback function that is provided as parameter will be invoked for a newly arrived image and can be rendered by your application.
```cpp
class PreviewRender : public RealSenseID::PreviewImageReadyCallback
{
public:
	void OnPreviewImageReady(const Image image)
	{
		// Callback will be used to provide RGB preview image (for RAW10_1080P PreviewMode - raw converted to RGB).
	}

    	void OnSnapshotImageReady(const Image image) // Not mandatory. To enable it, see Device Configuration API.
	{
		// Callback will be used to provide images destined to be dumped (cropped/full). See DeviceConfig::DumpMode attribute.
		// Raw formatted images will be raised by this function.
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

### Python API (new)
Starting with version 0.22.0 we provide python API:


```python
""" 
Authenticate example
"""
import rsid_py

def on_result(result, user_id):    
    if result == rsid_py.AuthenticateStatus.Success:
        print('Authenticated user:', user_id)

with rsid_py.FaceAuthenticator("COM9") as f:
    f.authenticate(on_result=on_result)

```


Please visit the python [samples](./samples/python) page for details and samples.


## Product Keys
Users need to obtain product/license key from the RealSense licensing portal. If no license is available the device will only 
accept basic face detection operations. Product keys are tied to specific device serial numbers. The activation process is done through
the RealSense licensing portal

Once you have a product key that is activated in the protal for your device, you need to provide the key to the device through one of the following ways:

**RealSenseID Tool (GUI or CLI)**:
1. In the cli `rsid-cli`: use the option `l` to pass the product key.
2. In the `rsid-viewer`: you need to click settings, click `Activate License` and enter the product key.

NOTE:
  - Device needs to be connected while providing the product key to the cli/gui tools.
  - Host needs access to the internet while providing the product key to the cli/gui tools.

**Product/License Key API (Programatically):**
1. If you're develping a custom app, you can use the `Authenticator::SetLicenseKey` and `Authenticator::ProvideLicense` APIs to 
supply the product key. More details: [FaceAuthenticator.h](include/RealSenseID/FaceAuthenticator.h)

For non-perpetual licenses, the device may respond with `Status::LicenseCheck` to indicate that the license needs re-validation. 
In such cases, the host must perform a license validation session with the cloud-based license server and the device.
See the `EnableLicenseCheckHandler(..)` and `DisableLicenseCheckHandler()` in [FaceAuthenticator.h](include/RealSenseID/FaceAuthenticator.h) for details.

NOTE:
  - Device needs to be connected while providing the product key to the API.
  - Host needs access to the internet while providing the product key to the API.


**Globally Stored Product key:**
1. You may also use the global configuration  (Registry on Windows, config files on Linux)


## Setting Product Key Programatically
### Using the authenticator api 
```c++
authenticator->SetLicenseKey(license_key);
```
This will set the license key for the current session in memory.

## Deployment / Persisting Product Keys
In order to handle deployment and not having to enter the license key through the tools and/or the API, users may opt to choose to persist/store
the product key so that it can be picked up when needed by the SDK. This can be done by setting the product key in the resitry (Windows) or in 
configuration files (Linux). The steps below desribe the process to perform this opteration.

When using this option, developers do not need to call the `SetLicenseKey` API. Developers must call the `ProvideLicense` at least once.
For furhter information, please refer to the API documentation for `ProvideLicense` in [FaceAuthenticator.h](include/RealSenseID/FaceAuthenticator.h)

### Setting Product Key on Windows
Set license/product key in Windows using PowerShell as follows:
* Open PowerShell <b>as administrator</b>
* Create registry Key
```
New-Item -Path "HKLM:\Software\Intel\VisionPlatform"
```
* Set registry value example
```
Set-ItemProperty -Path "HKLM:\Software\Intel\VisionPlatform" -Name "License" -Value "ac87f323-1a91-4959-abe8-488fb5df5f12"
```
* Verify that the value has been correctly set by issuing the command:
```
Get-ItemPropertyValue  -Path "HKLM:\Software\Intel\VisionPlatform\" -Name License
```
The license key should be printed out.

### Setting Product Key on Linux
Same as with Windows, users need to obtain the license key from the RealSense licensing portal. If the user obtains a 
license key `ac87f323-1a91-4959-abe8-488fb5df5f12`, user can set the license/product key as follows:

The library will check for a `license.json` file in the following directories in the following order: 
1. In user home folder: `~/.intel/visionplatform/license.json`
2. In system folder: `/etc/intel/visionplatform/license.json` 

If the file is present and readable in any of the directories, the file will be parsed and the `license_key` will be
utilized. 
The file should look like the following:
```json
{
    "license_key": "ac87f323-1a91-4959-abe8-488fb5df5f12"
}
```

NOTE: When running with `sudo`, the home directory path will be `/root/.intel/visionplatform/license.json`. 
In which case it is advisable to use the system path `/etc/intel/..` instead of the user path `~/.intel/..` 

## Notes on Deployment with Perpetual/Subscription Product Key Types
Perpetual Product Keys are handled differently that subscription-based Product Keys. While subscription-based keys will trigger
a license check after a few thousand of API calls, perpetual keys will never trigger a license check once installed to the device.

Developers are encouraged to use the `ProvideLicense` API before deploying the devices to the field. For developers with perpetual product keys 
the devices will continue operation without having to perform any license checks and will not require internet connection.

Additionally, developers with subscription-based keys should validate that their setup is functional by calling `ProvideLicense` before 
the deployment starts. It is also important for developers with subscription-based keys to:

1. Ensure that the product key is stored in the host.
2. Ensure that the host has internet connectivity in the deployment locations.


## UpdateChecker API
You can use the [UpdateChecker](include/RealSenseID/UpdateChecker.h) API to obtain latest release info from the web. 

The following example verifies whether a new firmware or host library is available, and prints the release and release notes URLs.

```c++
void check_for_updates(const RealSenseID::SerialConfig& serial_config)
{
    auto update_checker = RealSenseID::UpdateCheck::UpdateChecker();
    RealSenseID::UpdateCheck::ReleaseInfo remote {};
    RealSenseID::UpdateCheck::ReleaseInfo local {};    
    auto status1 = update_checker.GetRemoteReleaseInfo(remote);
    auto status2 = update_checker.GetLocalReleaseInfo(serial_config, local);
    if (status1 != RealSenseID::Status::Ok || status2 != RealSenseID::Status::Ok)
    {
       throw std::runtime_error("Failed to fetch release info");
    }

    bool update_available = (remote.sw_version > local.sw_version) || (remote.fw_version > local.fw_version);
    if (update_available)
    {
        std::cout << "Update available\n";
        std::cout << " * Release notes: " << remote.release_notes_url << std::endl;
        std::cout << " * Update URL: " << remote.release_url << std::endl;
    }
    else
    {
        std::cout << "No updates available\n";
    }
}
```

## Proxy Setup
License verification and UpdateChecker are built on top of libcurl and will honor the curl environment variables proxy settings:

Linux:
``` console
export http_proxy="http://user:pwd@proxy:port"
export https_proxy="http://user:pwd@proxy:port"
```

Windows PowerShell:
``` console
$env:http_proxy="http://user:pwd@proxy:port"
$env:https_proxy="http://user:pwd@proxy:port"
```


3. Please refer to the main documentation for libcurl proxy configuration here: [Everything CURL/Proxies](https://everything.curl.dev/transfers/conn/proxies)

## License
This project is licensed under Apache 2.0 license. Relevant license info can be found in "License Notices" folder.

## Intel RealSense ID F450 and F455 Architecture Diagram
![plot](./docs/F450_Architecture.png?raw=true)
