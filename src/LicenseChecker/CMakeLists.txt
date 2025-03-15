set(RSID_LICENSE_CHECKER "rsid-license-checker")

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(LICENSE_LIB_HEADERS
        "${SRC_DIR}/LicenseChecker.h"
        "${SRC_DIR}/LicenseData.h"
        "${SRC_DIR}/LicenseUtils.h"
        "${SRC_DIR}/LicenseUtilsImpl/ILicenseUtils.h"
        "${SRC_DIR}/LicenseCheckerImpl/ILicenseCheckerImpl.h"
        "${SRC_DIR}/LicenseCheckerImpl/LicenseCheckerImpl.h"
        "${SRC_DIR}/../Logger/Logger.h"
        )
set(LICENSE_LIB_SOURCES
        "${SRC_DIR}/LicenseChecker.cc"
        "${SRC_DIR}/LicenseUtils.cc"
        "${SRC_DIR}/LicenseCheckerImpl/LicenseCheckerImpl.cc"
        "${SRC_DIR}/../Logger/Logger.cc"
)

# Platform specific implementation
if(ANDROID)
    list(APPEND LICENSE_LIB_HEADERS "${SRC_DIR}/LicenseUtilsImpl/LicenseUtils_android.h")
    list(APPEND LICENSE_LIB_SOURCES "${SRC_DIR}/LicenseUtilsImpl/LicenseUtils_android.cc")
else()
    if(MSVC)
        list(APPEND LICENSE_LIB_HEADERS "${SRC_DIR}/LicenseUtilsImpl/LicenseUtils_win.h")
        list(APPEND LICENSE_LIB_SOURCES "${SRC_DIR}/LicenseUtilsImpl/LicenseUtils_win.cc")
        list(APPEND LICENSE_EXTRA_LIBS  winreg::headeronly)
    else()
        list(APPEND LICENSE_LIB_HEADERS "${SRC_DIR}/LicenseUtilsImpl/LicenseUtils_nix.h")
        list(APPEND LICENSE_LIB_SOURCES "${SRC_DIR}/LicenseUtilsImpl/LicenseUtils_nix.cc")
    endif()
endif()

add_library(${RSID_LICENSE_CHECKER} STATIC ${LICENSE_LIB_HEADERS} ${LICENSE_LIB_SOURCES})

set_target_properties(${RSID_LICENSE_CHECKER} PROPERTIES
        CXX_STANDARD 17
        CXX_EXTENSIONS OFF
        )

target_include_directories(${RSID_LICENSE_CHECKER} PRIVATE
        ${SRC_DIR}
        $<TARGET_PROPERTY:restclient-cpp,INCLUDE_DIRECTORIES>
        ${LICENSE_EXTRA_INCLUDES}
)

target_link_libraries(${RSID_LICENSE_CHECKER}
        PRIVATE
        spdlog::spdlog
        Threads::Threads
        nlohmann_json::nlohmann_json
        mvorbrodt_base64::headeronly
        restclient-cpp
        ${LICENSE_EXTRA_LIBS}
        )
		
set_common_compile_opts(${RSID_LICENSE_CHECKER})
