project(LibRealSenseID_Py CXX)

include(FetchContent)


if(POLICY CMP0148)
  cmake_policy(SET CMP0148 NEW)
endif()

FetchContent_Declare(
  pybind11
   URL https://github.com/pybind/pybind11/archive/refs/tags/v2.11.1.tar.gz
   URL_HASH SHA256=d475978da0cdc2d43b73f30910786759d593a9d8ee05b1b6846d1eb16c6d2e0c
)

FetchContent_MakeAvailable(pybind11)


set(PYBIND11_CPP_STANDARD -std=c++11)
set(PY_TARGET rsid_py)

set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

set(SOURCES 
    "${SRC_PATH}/rsid_py.cc" 
    "${SRC_PATH}/rsid_py.h"
    "${SRC_PATH}/face_auth_py.cc"    
    "${SRC_PATH}/discover_devices_py.cc"
    "${SRC_PATH}/device_controller_py.cc"
    "${SRC_PATH}/logging_py.cc"
    )

if (RSID_PREVIEW)
    list(APPEND SOURCES "${SRC_PATH}/preview_py.cc")    
endif ()

pybind11_add_module(${PY_TARGET} ${HEADERS} ${SOURCES})


target_link_libraries(${PY_TARGET} PRIVATE ${LIBRSID_CPP_TARGET})

set_target_properties(${PY_TARGET}
        PROPERTIES
        COMPILE_DEFINITIONS $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
        FOLDER "wrappers"
        )

if (RSID_SECURE)
    target_compile_definitions(${PY_TARGET} PUBLIC RSID_SECURE)
endif ()

if (RSID_PREVIEW)
    target_compile_definitions(${PY_TARGET} PUBLIC RSID_PREVIEW)
endif()

set_common_compile_opts(${PY_TARGET})

get_target_property(PROJECT_SOURCES ${PY_TARGET} SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${PROJECT_SOURCES})
