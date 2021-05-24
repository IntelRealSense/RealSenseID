project(LibRealSenseID)

add_library(${LIBRSID_CPP_TARGET} SHARED "")

find_package(Threads REQUIRED)

target_link_libraries(${LIBRSID_CPP_TARGET} 
    PRIVATE
        spdlog::spdlog
        Threads::Threads
        $<$<BOOL:${RSID_SECURE}>:mbedtls::mbedtls>
)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(HEADERS
    "${SRC_DIR}/CommonValues.h"
    "${SRC_DIR}/FaceAuthenticatorImpl.h"
    "${SRC_DIR}/DeviceControllerImpl.h"
    "${SRC_DIR}/StatusHelper.h"
)
set(SOURCES
    "${SRC_DIR}/FaceAuthenticator.cc"
    "${SRC_DIR}/FaceAuthenticatorImpl.cc"
    "${SRC_DIR}/DeviceController.cc"
    "${SRC_DIR}/DeviceControllerImpl.cc"
    "${SRC_DIR}/StatusHelper.cc"
    "${SRC_DIR}/Version.cc"
    "${SRC_DIR}/Logging.cc"
    "${SRC_DIR}/FwUpdater.cc"
    "${SRC_DIR}/DiscoverDevices.cc"
)


if(RSID_PREVIEW)
    target_compile_definitions(${LIBRSID_CPP_TARGET} PRIVATE RSID_PREVIEW)
    list(APPEND HEADERS "${SRC_DIR}/PreviewImpl.h")
    list(APPEND SOURCES "${SRC_DIR}/Preview.cc" "${SRC_DIR}/PreviewImpl.cc")
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    	target_link_libraries(${LIBRSID_CPP_TARGET} PRIVATE usb)
	    target_link_libraries(${LIBRSID_CPP_TARGET} PRIVATE uvc)
	    target_include_directories(${LIBRSID_CPP_TARGET} PUBLIC "${THIRD_PARTY_DIRECTORY}/uvc/include")
    endif()
    target_include_directories(${LIBRSID_CPP_TARGET}  PRIVATE "${THIRD_PARTY_DIRECTORY}/libjpeg-turbo_2_1_0")
    target_include_directories(${LIBRSID_CPP_TARGET}  PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/../3rdparty/libjpeg-turbo_2_1_0")        
    target_link_libraries(${LIBRSID_CPP_TARGET} PRIVATE jpeg-static)        
    add_subdirectory("${SRC_DIR}/Capture")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    find_library(ANDROID_LOG log)
    target_link_libraries(${LIBRSID_CPP_TARGET} PUBLIC ${ANDROID_LOG})
endif()

target_sources(${LIBRSID_CPP_TARGET} PRIVATE ${HEADERS} ${SOURCES})

target_include_directories(${LIBRSID_CPP_TARGET} 
    PRIVATE 
        "${SRC_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/Logger"        
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)


target_compile_definitions(${LIBRSID_CPP_TARGET}
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
        $<$<BOOL:${RSID_DEBUG_VALUES}>:RSID_DEBUG_VALUES>
    PUBLIC
        $<$<BOOL:${RSID_SECURE}>:RSID_SECURE>
)

set_target_properties(PROPERTIES DEBUG_POSTFIX ${RSID_DEBUG_POSTFIX})

add_subdirectory("${SRC_DIR}/Logger")
add_subdirectory("${SRC_DIR}/PacketManager")
add_subdirectory("${SRC_DIR}/Matcher")
add_subdirectory("${SRC_DIR}/FwUpdate")

# set ide source group
get_target_property(PROJECT_SOURCES ${LIBRSID_CPP_TARGET} SOURCES)
source_group(TREE "${SRC_DIR}" FILES ${PROJECT_SOURCES})

# show api folder in ide
file(GLOB API_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/../include/RealSenseID/*.h")
add_custom_target("include" SOURCES ${API_HEADERS})