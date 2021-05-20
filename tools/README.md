
# 			Intel RealSense ID Tools

[Instructions for Windows](#windows----compilation-and-usage)

[Instructions for Linux](#linux----compilation-and-usage)

[Instructions for Android](#android----compilation-and-usage)

## **Windows** -  Compilation and usage 

##  **Dependencies**:

 -  **[CMake](https://cmake.org/)**
-   **[Visual Studio](https://visualstudio.microsoft.com/downloads/)**


##  **How to build:**
Use CMake version 3.13.0 or above
1. Download or Clone the repository
2. In cloned folder, open command line

	**Default build:**
	```console
	> mkdir build
	> cd build
	> cmake ..
	```
	**Build with secure communication enabled:**
	```console
	> mkdir build
	> cd build
	> cmake -DRSID_SECURE=1 ..
	```
	**Build with preview enabled:**
	```console
	> mkdir build
	> cd build
	> cmake -DRSID_PREVIEW=1 ..
	```

3. In build folder you should have full visual studio solution. Run RealSenseID.sln
4. Build solution
5. After building solution you will find in \build\bin\<Debug/Release> three executables:
	- rsid-viewer.exe: GUI to view and use RealsenseID
	- rsid-cli.exe: Command line tool to use RealSenseID.
	- fw-updater-cli.exe: Firmware update tool
    

**Done!**
## **How to run example apps**

***Make sure you have connected the Intel RealSense ID F450 / F455 device to your PC***.

Run Windows "Device Manager" and check in which COM port the device was recognized (It should appear under **Ports (COM & LPT)** (For example: COM3)

###  **RealSenseID Viewer:**
1. Simply Run: **rsid-viewer.exe**
2. Further configuration can be found in  **"rsid-viewer.exe.config"** file located next to the .exe file.

#### Dumping  
For saving images from sensors stream you can enable "Dump Frames" under app's setting screen. notice that:
- Dumping is running only in authentication,enroll or faceprints session.  
- Images will be saved under the directory specified in "DumpDir" attribute in  **"rsid-viewer.exe.config"** in PNG/RAW format.  
- If metadata enabled (see "sensor timestamps" in repository readme):  
 images will be named by sensor timestamp and the main image used by algorithm will be named with the suffix "selected".  
- Otherwise, images will be named by a frame number only.  


###  **RealSenseID Command Line Tool:**
At bin folder (where .exe files are located) run command line and run the app as following:
```console
rsid-cli.exe <port>
```
For Example:
```console
rsid-cli.exe COM3 
```


## **Linux** -  Compilation and usage 

***  instructions for Ubuntu 18.4 ***

##  **Dependencies**:

 -  **[CMake](https://cmake.org/)** - 3.13.0 or higher.

##  **How to compile**:

 1.  Download or Clone the repository.
 2. In main folder, Open Terminal and run the following:
 ```console
 > mkdir build
 > cd build
 > cmake ..
 > make
 ```
3. After building solution you will find in \build\bin\ two executables:
	1. rsid-cli: Command line interface to RealSenseID.
    2. fw-updater-cli: Firmware update tool.
    

**Done!**

## **How to run toos**

***First make sure you have connected the Intel RealSense ID F450 / F455 Camera to your PC***.

1. Identify your device by running:
	```console 
	sudo dmesg | grep 'Intel F450' -A 5 | grep ttyACM
	```
	The output should be something like: "... ttyACM#: USB ACM device ..." - where # is the port number.
    
2. Grant the necessary permissions for your user, to be able to communicate with the device:
	```console
	sudo usermod -a -G dialout $USER
	```
	Log out of your current session for this to take effect.

   Alternatively, you can run the following command, replacing # with the port number found in step 1:
	```console
	sudo chmod 666 /dev/ttyACM#
	```
	This will only affect the current session.

###  **RealSenseID Command Line Interface:**
At bin folder (where .exe files are located) run command line and run the app as following:
```console
./rsid-cli <port> 
```
For Example:
```console
./rsid-cli /dev/ttyACM0 usb
```


## **Android** -  Compilation and usage 

## **Dependencies**:
- **[Android Studio](https://developer.android.com/studio)**
- **[SWIG](http://www.swig.org/download.html)**
- **[LibUSB](https://github.com/libusb/libusb)**
(Currently required - later will only be required to enable preview)
- **[LibUVC](https://github.com/libuvc/libuvc)**
(Currently required - later will only be required to enable preview)

##  **How to build:**
### **Prerequisites**
- `SWIG` is used to create the JNI and Java wrappers to RealSenseID C++ API. Please download and install SWIG-4.0.2 or newer in your development environment to enable wrapper code generation during CMake builds. If developing in Windows, it's suggested to download swigwin-4.0.2 or later, install it and add it to PATH environment variable. For other development OSes, please follow instructions [online](http://www.swig.org/Doc4.0/SWIGDocumentation.html#Preface_installation) on how to compile and install from source.
- `LIBUSB & LIBUVC` are required for preview in Android. Currently, preview support must be part of Android compilation (RSID_PREVIEW flag must be set to ON). The developer can choose if to compile both libraries from source for Android OS and the [ABI](https://developer.android.com/ndk/guides/abis) matching the destination platform, or take the libraries we precompiled and shared in the [releases section](https://github.com/IntelRealSense/RealSenseID/releases).

**Build sample APK**
 1. Download or Clone the repository.
 2. Copy LibUSB and LibUVC header folder (include) and precompiled binaries devided by ABI folders (x86_64, arm64-v8a etc...) to \<RealSenseID repository\>/3rdparty/uvc. It should look similar to this: ![3rdparty/uvc](rsid-android-app/3rdparty_uvc.png)
 3. In Android Studio, File->Open: \<RealSenseID repository\>/tools/rsid-android-app
 4. "Sync project" and then "Make project".
 
 	Note: Application and "RealSenseIDSwigClient" module have two flavors. "Secured" and "Unsecured" to choose between the two supported modes. If when trying to switch between them, you suffer compilation issues, try Android Studio's "Refresh Linked C++ Projects" option. Another thing that might work is to remove all auto generated files (try "git clean -dfix" can help).

 **Done!**
## **How to run the android application**
- Install the sample application on an Android device.
- Connect Intel RealSense ID F450 / F455 device to an OTG supported Android device.
- Run the sample application and when asked for, grant the application permission to access the Realsense ID F45# device.
