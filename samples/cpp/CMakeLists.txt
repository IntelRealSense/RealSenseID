cmake_minimum_required(VERSION 3.10.2)
project(RealSenseID_CPP_Samples CXX)

if(NOT RSID_SECURE)
    # authenticate
    add_executable(authenticate-cpp-sample authenticate.cc)
    target_link_libraries(authenticate-cpp-sample PRIVATE rsid)
    set_target_properties(authenticate-cpp-sample PROPERTIES FOLDER "samples/cpp")

    # enroll
    add_executable(enroll-cpp-sample enroll.cc)
    target_link_libraries(enroll-cpp-sample PRIVATE rsid)
    set_target_properties(enroll-cpp-sample PROPERTIES FOLDER "samples/cpp")

    # host-mode
    add_executable(host-mode-cpp-sample host-mode.cc)
    target_link_libraries(host-mode-cpp-sample PRIVATE rsid)
    set_target_properties(host-mode-cpp-sample PROPERTIES FOLDER "samples/cpp")    
    
else() # secure mode samples
    add_executable(pair-device-cpp-sample pair-device.cc secure_mode_helper.cc secure_mode_helper.h)
    target_link_libraries(pair-device-cpp-sample PRIVATE rsid mbedtls::mbedtls)    
    set_target_properties(pair-device-cpp-sample PROPERTIES FOLDER "samples/cpp")    
endif()

if (RSID_PREVIEW)
    # simple preview
    add_executable(preview-cpp-sample preview.cc)
    target_link_libraries(preview-cpp-sample PRIVATE rsid)
    set_target_properties(preview-cpp-sample PROPERTIES FOLDER "samples/cpp")

    # snapshot preview
    add_executable(preview-snapshot-cpp-sample preview_snapshot.cc)
    target_link_libraries(preview-snapshot-cpp-sample PRIVATE rsid)
    set_target_properties(preview-snapshot-cpp-sample PROPERTIES FOLDER "samples/cpp")

    # face-rects
    add_executable(multi-faces-cpp-sample multi-faces.cc)
    target_link_libraries(multi-faces-cpp-sample PRIVATE rsid)
    set_target_properties(multi-faces-cpp-sample PROPERTIES FOLDER "samples/cpp")    
endif()