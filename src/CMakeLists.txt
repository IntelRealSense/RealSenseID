project(LibRealSenseID)

add_library(${LIBRSID_CPP_TARGET} SHARED "")

target_compile_options(${LIBRSID_CPP_TARGET} PUBLIC -DRSID_SECURE)
target_link_libraries(${LIBRSID_CPP_TARGET} 
                                            PRIVATE mbedtls::mbedtls
                                            PRIVATE spdlog::spdlog)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(HEADERS "${SRC_DIR}/FaceAuthenticatorImpl.h" "${SRC_DIR}/DeviceControllerImpl.h" "${SRC_DIR}/StatusHelper.h")
set(SOURCES "${SRC_DIR}/FaceAuthenticator.cc" "${SRC_DIR}/FaceAuthenticatorImpl.cc" "${SRC_DIR}/DeviceController.cc"
            "${SRC_DIR}/DeviceControllerImpl.cc" "${SRC_DIR}/StatusHelper.cc" "${SRC_DIR}/Version.cc" "${SRC_DIR}/Logging.cc" 
	    "${SRC_DIR}/Matcher.cc" $<$<CXX_COMPILER_ID:MSVC>:${SRC_DIR}/DiscoverDevices.cc>)


if(RSID_PREVIEW)
    target_compile_definitions(${LIBRSID_CPP_TARGET} PRIVATE RSID_PREVIEW)
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
        list(APPEND HEADERS "${SRC_DIR}/PreviewImpl.h"  "${SRC_DIR}/AndroidPreview.h")
        list(APPEND SOURCES "${SRC_DIR}/Preview.cc" "${SRC_DIR}/PreviewImpl.cc" "${SRC_DIR}/AndroidPreview.cc")
        target_link_libraries(${LIBRSID_CPP_TARGET} PRIVATE usb)
        target_link_libraries(${LIBRSID_CPP_TARGET} PRIVATE uvc)
        target_include_directories(${LIBRSID_CPP_TARGET}  PUBLIC "${THIRD_PARTY_DIRECTORY}/uvc/include")
    else()    
        list(APPEND HEADERS "${SRC_DIR}/PreviewImpl.h" "${SRC_DIR}/OpencvPreview.h")
        list(APPEND SOURCES "${SRC_DIR}/Preview.cc" "${SRC_DIR}/PreviewImpl.cc" "${SRC_DIR}/OpencvPreview.cc")
        target_link_libraries(${LIBRSID_CPP_TARGET} PRIVATE opencv)
        target_copy_opencv_files(${LIBRSID_CPP_TARGET})
    endif()
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	find_library(ANDROID_LOG log)
	target_link_libraries(${LIBRSID_CPP_TARGET} PUBLIC ${ANDROID_LOG})
endif()


target_sources(${LIBRSID_CPP_TARGET} PRIVATE ${HEADERS} ${SOURCES})

target_include_directories(${LIBRSID_CPP_TARGET} PRIVATE "${SRC_DIR}")
target_include_directories(${LIBRSID_CPP_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Logger")
target_include_directories(${LIBRSID_CPP_TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")

set_target_properties(${LIBRSID_CPP_TARGET} PROPERTIES DEBUG_POSTFIX ${RSID_DEBUG_POSTFIX})

# parallel build in msvc
target_compile_options(${LIBRSID_CPP_TARGET} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/MP>)
# set cpp version in the __cplusplus macro in msvc
target_compile_options(${LIBRSID_CPP_TARGET} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>)

add_subdirectory("${SRC_DIR}/Logger")
add_subdirectory("${SRC_DIR}/PacketManager")

# set ide source group
get_target_property(PROJECT_SOURCES ${LIBRSID_CPP_TARGET} SOURCES)
source_group(TREE "${SRC_DIR}" FILES ${PROJECT_SOURCES})

# show api folder in ide
file(GLOB API_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/../include/RealSenseID/*.h")
add_custom_target("include" SOURCES ${API_HEADERS})
