# Python samples. 

#### Instructions
* Build the python wrapper from the root directory ```cmake -DRSID_PY=ON -DRSID_PREVIEW=ON ..```
* Copy the `rsid.dll` and `rsid_py*.pyd` (or `rsid.so` and `rsid_py*.so` if in Linux) from the build directory to this directory.
* Edit the ```PORT``` variable in the samples to point to the right serial port (.e.g. ```COM9``` in Windows or `/dev/ttyACM0` in Linux)

#### Samples
* [authenticate.py](authenticate.py): basic sample to authenticate a user. 
* [enroll.py](enroll.py): basic sample to enroll a user.
* [enroll_image.py](enroll_image.py): basic sample to enroll a user using an image file (requires opencv).
* [users.py](users.py): display enrolled users.
* [preview.py](preview.py): getting preview images from the camera.
* [host_mode.py](host_mode.py): host mode where face features are stored in the host instead of the device.
* [viewer.py](viewer.py): nice little GUI to authenticate/enroll/delete users (requires opencv and numpy).

