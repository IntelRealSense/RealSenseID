project(LibRealSenseID_Py CXX)

# clone pybind to the build folder
set(PYBIND_BUILD_DIR "${CMAKE_BINARY_DIR}/external-projects/pybind11")
configure_file(${CMAKE_SOURCE_DIR}/3rdparty/pybind11/CMakeLists.txt ${PYBIND_BUILD_DIR}/CMakeLists.txt)
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY "${PYBIND_BUILD_DIR}"
)
# build it
execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${PYBIND_BUILD_DIR}"
)

# Add pybind11 makefile
add_subdirectory("${CMAKE_BINARY_DIR}/3rdparty/pybind11"
                 "${CMAKE_BINARY_DIR}/3rdparty/pybind11"
                 EXCLUDE_FROM_ALL)

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



get_target_property(PROJECT_SOURCES ${PY_TARGET} SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${PROJECT_SOURCES})
