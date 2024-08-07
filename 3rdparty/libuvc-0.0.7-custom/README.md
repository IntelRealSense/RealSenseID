## RealSenseID Modified `libuvc`

This folder contains a slightly modified version of `libuvc`.

Changes:

1. Added format `UVC_FRAME_FORMAT_W10` in `stream.c`
2. Added debugging controlled by macro `RSID_W10_DEBUG` in `stream.c` (Default: disabled/undef). 
Define this macro to trace libuvc stream selection.
3. CMake: Disable jpeg/libjpeg
4. CMake: Disable shared library


**NOTE**: Search for `RSID_W10` in this folder for each change that was done to the original library.

Extra resources for future reference:
1. Example of adding a custom format: https://github.com/pupil-labs/libuvc/commit/77de9a5310a7f281baafef4b385858ba96da43be
2. Example of Windows support: https://github.com/pupil-labs/libuvc/commit/b7181e9625fb19eae80b54818537d2fd0a9090d3 

---

`libuvc` is a cross-platform library for USB video devices, built atop `libusb`.
It enables fine-grained control over USB video devices exporting the standard USB Video Class
(UVC) interface, enabling developers to write drivers for previously unsupported devices,
or just access UVC devices in a generic fashion.

## Getting and Building libuvc

Prerequisites: You will need `libusb` and [CMake](http://www.cmake.org/) installed.

To build, you can just run these shell commands:

    git clone https://github.com/libuvc/libuvc
    cd libuvc
    mkdir build
    cd build
    cmake ..
    make && sudo make install

and you're set! If you want to change the build configuration, you can edit `CMakeCache.txt`
in the build directory, or use a CMake GUI to make the desired changes.

There is also `BUILD_EXAMPLE` and `BUILD_TEST` options to enable the compilation of `example` and `uvc_test` programs. To use them, replace the `cmake ..` command above with `cmake .. -DBUILD_TEST=ON -DBUILD_EXAMPLE=ON`.
Then you can start them with `./example` and `./uvc_test` respectively. Note that you need OpenCV to build the later (for displaying image).

## Developing with libuvc

The documentation for `libuvc` can currently be found at https://libuvc.github.io/.

Happy hacking!
