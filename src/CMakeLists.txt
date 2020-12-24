project(LibRealSenseID)

add_library(${LIBRSID_CPP_TARGET} SHARED "")

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(HEADERS "${SRC_DIR}/FaceAuthenticatorImpl.h" "${SRC_DIR}/DeviceControllerImpl.h" "${SRC_DIR}/StatusHelper.h")
set(SOURCES "${SRC_DIR}/FaceAuthenticator.cc" "${SRC_DIR}/FaceAuthenticatorImpl.cc" "${SRC_DIR}/DeviceController.cc"
            "${SRC_DIR}/DeviceControllerImpl.cc" "${SRC_DIR}/StatusHelper.cc" "${SRC_DIR}/Version.cc")

if(RSID_PREVIEW)
    list(APPEND HEADERS "${SRC_DIR}/PreviewImpl.h" "${SRC_DIR}/OpencvPreview.h")
    list(APPEND SOURCES "${SRC_DIR}/Preview.cc" "${SRC_DIR}/PreviewImpl.cc" "${SRC_DIR}/OpencvPreview.cc")
    target_compile_definitions(${LIBRSID_CPP_TARGET} PRIVATE RSID_PREVIEW)
    target_link_libraries(${LIBRSID_CPP_TARGET} PRIVATE opencv)
    target_copy_opencv_files(${LIBRSID_CPP_TARGET})
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
