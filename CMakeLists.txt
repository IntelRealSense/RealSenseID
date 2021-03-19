cmake_minimum_required(VERSION 3.10.2)

include(cmake/Version.cmake)
rsid_extract_version()
project(RealSenseID VERSION ${RSID_VERSION})

# C++14 and above
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# CCache
find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    message(STATUS "CCache found!")
endif ()

# Global properties
set(LIBRSID_CPP_TARGET rsid)
set(LIBRSID_C_TARGET rsid_c)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_build")
set(RSID_DEBUG_POSTFIX _debug)

# ---------------------------------------------------------------------------------------
# Set RSID_MASTER_PROJECT to ON if we are not used via add_subdirectory, but allow overriding
# ---------------------------------------------------------------------------------------
if(NOT DEFINED RSID_MASTER_PROJECT)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(RSID_MASTER_PROJECT ON)
    else()
        set(RSID_MASTER_PROJECT OFF)
    endif()
endif()

# ---------------------------------------------------------------------------------------
# Set default build to release
# ---------------------------------------------------------------------------------------
if(RSID_MASTER_PROJECT AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})

# Options
option(RSID_DEBUG_CONSOLE "Log everything to console" ON)
option(RSID_DEBUG_FILE "Log everything to rsid_debug.log file" OFF)
option(RSID_DEBUG_SERIAL "Log all serial communication" OFF)
option(RSID_DEBUG_VALUES "Replace default common values with debug ones" OFF)
option(RSID_C_CLIENT "Build c client" ON)
option(RSID_PREVIEW "Enable opencv preview" OFF)
option(RSID_EXAMPLES "Build examples" ON)
option(RSID_TIDY "Enable clang-tidy" OFF)
option(RSID_DOXYGEN "Build doxygen docs" OFF)
option(RSID_SECURE "Enable secure communication with device" OFF)
option(RSID_TOOLS "Build additional tools" ON)

if(RSID_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    message(STATUS "Enabled clang-tidy")
endif()

set(THIRD_PARTY_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty")

# # Paths for lib, bin, install, etc.
# if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Android")
#     set(BIN_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
#     set(LIBS_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
#     set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BIN_OUTPUT_PATH}")
#     set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${LIBS_OUTPUT_PATH}")
#     set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${dS_OUTPUT_PATH}")
#     set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
# endif()

# Add 'cmake' folder to CMake's module lookup paths.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

include(OS)
include(SpdLog)

if(RSID_SECURE)
    include(Mbedtls)
endif()

if(RSID_PREVIEW)
    if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
        include(UVC)
    else()
        include(OpenCV)
    endif()
endif()

if(RSID_EXAMPLES)
    add_subdirectory(examples)
endif()

if(RSID_DOXYGEN)
    include(Doxygen)
endif()

if (RSID_TOOLS)
    add_subdirectory(tools)
endif ()

add_subdirectory(src)
add_subdirectory(wrappers)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(RSID_VERSION_STRING "${RSID_VERSION_MAJOR}.${RSID_VERSION_MINOR}.${RSID_VERSION_PATCH}")

write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/rsidConfigVersion.cmake"
    VERSION ${RSID_VERSION_STRING} COMPATIBILITY AnyNewerVersion)

configure_package_config_file(cmake/rsidConfig.cmake.in rsidConfig.cmake
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/rsid/"
    INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/bin
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/rsidConfig.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/rsid/"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/rsidConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/rsid/"
)
