# Intel RealSense ID Solution Vision Library for Facial Authentication
---
## Overview
**Intel® RealSenseID** is a cross-platform library providing Facial Authentication capabilities to applications, using Intel® F450 Device.

Anti-Spoofing and Face recognition NN algorithms run on the device
to achieve a highly-secure and state of the art facial-authentication.
 
Our solution is fully secured including secured boot, signing verification
and mutual authentication - encrypted communication between the host and the device based on ECDH protocol.

## F450 Device
Intel® F450 is a plug-and-play, easy to use, independent SoC,
with Proprietary dual-sensor camera specific for face authentication needs.
SOC has Neural Processing Unit which boosts NN processing and Secure Element to provide security features.

For high-level architecture see [F450 architecure diagram](#f450-architecure-diagram).

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
    

 
## Start here
This snippet shows the basic usage of our library, it shows 
how to enroll a user, authenticate a user, and remove a user from the device's storage:

```cpp    
// Create face authenticator instance and connect to the device on COM9    
RealSenseID::FaceAuthenticator auth {&sig_clbk};
auto connect_status = authenticator.Connect({RealSenseID::SerialType::USB, "COM9"}); // RealSenseID::SerialType::UART can be used in case UART I/F is required
      
// Enroll a user
const char* user_id = "John";
my_enroll_clbk enroll_clbk;
auto enroll_status = authenticator.Enroll(enroll_clbk, user_id);    

// Authenticate a user
my_auth_clbk auth_clbk;
auto auth_status = authenticator.Authenticate(auth_clbk);    
    
// Remove the user from device
bool success = authenticator.RemoveUser(user_id);

// Disconnect from the device
authenticator.Disconnect();
```
For additional languages,build instruction and detailed code please see our [examples](./examples).

## RealSense ID API
C, C++ and C# API wrappers are provided as part of the library.	
Java and python API under development. All APIs are synchronous and assuming a new operation will not be activated till previous is completed.


C++ [API](./include/RealSenseID).

C/C# [API](./wrappers).


### Main FA API - FaceAuthenticator

##### Connect
Connects host library to F450 device using USB (```RealSenseID::SerialType::USB```) or UART interfaces (```RealSenseID::SerialType::UART```).
```cpp    
// Create face authenticator instance and connect to the device on COM9
RealSenseID::FaceAuthenticator authenticator(&sig_clbk);
// connect to F450 using USB interface 
auto connect_status = authenticator.Connect({RealSenseID::SerialType::USB, "COM9"}); 
// Connect to F450 using UART interface 
auto connect_status = authenticator.Connect({RealSenseID::SerialType::UART, "COM9"});
```

##### Enrollment
Starts camera, runs NN algo(s) pipeline and saves your facial features to secured flash on F450 device, stored feartures will be matched later during authentication.
For best performance, do this in normal lighting conditions, and look directly at the camera. During the enrollment process hint statuses might be sent to the callback provided by application.
Full list of the hints can be found in [EnrollStatus.h](./include/RealSenseID/).
```cpp    
// Enroll a user with user id - "John"
const char* user_id = "John";
my_enroll_clbk enroll_clbk;  // callback function will be invoked in case of a failure (e.g face not detected) or success
auto enroll_status = authenticator.Enroll(enroll_clbk, user_id);    
```

##### Authenticate
Single authentication attempt: Starts camera, runs NN algo(s) pipeline, calculates your facial features and compares them to all previously stored features in database in F450 secured flash.
Finally, returns whether the authentication was forbidden or allowed with pre-enrolled user id.
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

my_auth_clbk auth_clbk; // callback function will be invoked in case of a failure (e.g face not detected) or success
// Authenticate a user
auto auth_status = authenticator.Authenticate(auth_clbk);    
```

##### AuthenticateLoop
Continuous authentication till Cancel() API is invoked: 
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

my_auth_clbk auth_clbk; // callback function will be invoked in case of a failure (e.g face not detected) or a successful authentication of an user.
// start continuous authentication
auto auth_status = authenticator.AuthenticateLoop(auth_clbk);    
```

##### Cancel
Stops authentication. Authenticate or AuthenticateLoop calls supposed to return.
```cpp    
authenticator.Cancel();   
```   

##### RemoveAll
Removes all users stored in the device database.		
```cpp    
bool success = authenticator.RemoveAll();
```


##### RemoveUser
Removes a specific user from the device database.
```cpp    
// Enroll a user
const char* user_id = "John";    
// Remove the user "John" from device
bool success = authenticator.RemoveUser(user_id);
```

### Secure communication
The communication over the serial line with the device is encrypted, using the ECDH protocol for the initial key exchange.

Signature callbacks enables you to sign and validate communication with the device using your own public/private keys.
```cpp    
RealSenseID::Examples::SignClbk sig_clbk; 
RealSenseID::FaceAuthenticator auth {&sig_clbk};
```
   Here, the ```FaceAuthenicator``` is constructed with an example callback object to verify and generate signatures.

   Please see the [signature callback example](examples/shared/signature_example/rsid_signature_example.cc) for a detailed example.

### Preview API
Requiring OpenCV installed on the host device.
Currently only VGA (640x480) YUV format is available.

##### StartPreview
Starts F450 camera preview. Callback function provided as parameter will be invoked for a newly arrived image and can be rendered by your application.
```cpp
class PreviewRender : public RealSenseID::PreviewImageReadyCallback
{
public:
	void OnPreviewImageReady(const Image image){
	{
		// render image
	}
};

PreviewRender image_clbk; // callback function will be invoked once new image arrived from F450
// start continuous authenticatio
Preview preview;
auto status = preview.StartPreview(image_clbk);    
...
status = preview.StopPreview();    
```


##### PausePreview
Pause the camera preview.
```cpp
status = preview.PausePreview();    
```

##### ResumePreview
Resume the paused preview.
```cpp
status = preview.ResumePreview();    
```
##### StopPreview
Stops camera preview.
```cpp
status = preview.StopPreview();    
```

### DeviceController API
##### Connect
Connects host library to F450 device using USB (RealSenseID::SerialType::USB) or UART interfaces (RealSenseID::SerialType::UART).

##### Reboot
Resets F450 device.

##### Upgrade (Not available yet)
For now FRM.exe tool should be used for FW upgrades. In next releases FW upgrade will be part of Real Sense ID library.


## License
This project is licensed under Apache 2.0 license. Relevant license info can be found in "License Notices" folder

## Installing drivers (Windows only)

[**iCatch USB Driver to burn FW**] required by FRM.exe


[**MCP Utility to enable F450 UART on Windows OS **](https://www.microchip.com/developmenttools/ProductDetails/ADM00559)


## F450 Architecure Diagram
![plot](./docs/F450_Architecture.png?raw=true)
