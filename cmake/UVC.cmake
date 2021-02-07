add_library(usb SHARED IMPORTED GLOBAL)
set_target_properties(usb PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/uvc/${ANDROID_ABI}/libusb1.0.so")
add_library(uvc SHARED IMPORTED GLOBAL)
set_target_properties(uvc PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/uvc/${ANDROID_ABI}/libuvc.so")