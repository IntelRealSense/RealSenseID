include(FetchContent REQUIRED)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

FetchContent_Declare(
	winreg
	URL https://github.com/GiovanniDicanio/WinReg/archive/refs/tags/v6.1.1.tar.gz
	URL_HASH  SHA256=44CC0F6B3E75BF8B8167F7E96E97E0CBD0DA3CA2D622CFC72A37DA8FC0F395C0
        
)

if(NOT winreg_POPULATED)
    message(STATUS "Populating WinReg")
    FetchContent_Populate(winreg)
    set(winreg_INCLUDE_DIR ${winreg_SOURCE_DIR})

    # Create imported target winreg::headeronly
    add_library(winreg::headeronly INTERFACE IMPORTED)
    set_target_properties(winreg::headeronly PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${winreg_INCLUDE_DIR}/WinReg")
endif()
