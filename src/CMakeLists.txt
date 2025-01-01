project(LibRealSenseID)

add_library(${LIBRSID_CPP_TARGET} SHARED "")

find_package(Threads REQUIRED)

target_link_libraries(${LIBRSID_CPP_TARGET} 
    PRIVATE
        spdlog::spdlog
        Threads::Threads
        $<$<BOOL:${RSID_SECURE}>:mbedtls>        
)

if(RSID_NETWORK)
target_link_libraries(${LIBRSID_CPP_TARGET} 
    PRIVATE        
        rsid-license-checker
        nlohmann_json::nlohmann_json
        restclient-cpp
)
endif()

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
if(RSID_NETWORK)	
	list(APPEND SOURCES "${SRC_DIR}/UpdateChecker.cc")
else()
    list(APPEND SOURCES "${SRC_DIR}/UpdateCheckerNotImpl.cc")
endif()


if(RSID_PREVIEW)
    target_compile_definitions(${LIBRSID_CPP_TARGET} PRIVATE RSID_PREVIEW)
    list(APPEND HEADERS "${SRC_DIR}/PreviewImpl.h")
    list(APPEND SOURCES "${SRC_DIR}/Preview.cc" "${SRC_DIR}/PreviewImpl.cc")    
    # libjpeg-turbo-install 
	if(NOT WIN32)
		add_dependencies(${LIBRSID_CPP_TARGET} libjpeg-turbo) 
		target_include_directories(${LIBRSID_CPP_TARGET} PRIVATE "${LIBJPEG_TURBO_INSTALL_DIR}/include")	
		target_link_directories(${LIBRSID_CPP_TARGET} PRIVATE "${LIBJPEG_TURBO_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}")	
		target_link_libraries(${LIBRSID_CPP_TARGET} PRIVATE LibUVC::UVC jpeg)
	endif()	
    add_subdirectory("${SRC_DIR}/Capture")
endif()


target_sources(${LIBRSID_CPP_TARGET} PRIVATE ${HEADERS} ${SOURCES})

target_include_directories(${LIBRSID_CPP_TARGET} 
    PRIVATE 
        "${SRC_DIR}"
        $<$<BOOL:${RSID_NETWORK}>:$<TARGET_PROPERTY:restclient-cpp,INCLUDE_DIRECTORIES>>
        "${CMAKE_CURRENT_SOURCE_DIR}/Logger"        
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)


target_compile_definitions(${LIBRSID_CPP_TARGET}
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
        $<$<BOOL:${RSID_DEBUG_VALUES}>:RSID_DEBUG_VALUES>
        $<$<BOOL:${RSID_DEBUG_PACKETS}>:RSID_DEBUG_PACKETS>
    PUBLIC
        $<$<BOOL:${RSID_SECURE}>:RSID_SECURE>
)

set_target_properties(PROPERTIES DEBUG_POSTFIX ${RSID_DEBUG_POSTFIX})
set_common_compile_opts(${LIBRSID_CPP_TARGET})

add_subdirectory("${SRC_DIR}/Logger")
add_subdirectory("${SRC_DIR}/PacketManager")
add_subdirectory("${SRC_DIR}/Matcher")
add_subdirectory("${SRC_DIR}/FwUpdate")

if(RSID_NETWORK)
    add_subdirectory("${SRC_DIR}/LicenseChecker")
endif()

# set ide source group
get_target_property(PROJECT_SOURCES ${LIBRSID_CPP_TARGET} SOURCES)
source_group(TREE "${SRC_DIR}" FILES ${PROJECT_SOURCES})

# show api folder in ide
file(GLOB API_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/../include/RealSenseID/*.h")
add_custom_target("include" SOURCES ${API_HEADERS})
