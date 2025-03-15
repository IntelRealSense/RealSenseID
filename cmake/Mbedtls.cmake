include(FetchContent REQUIRED)

function(fetch_mbedtls)
    FetchContent_Declare(
            mbedtls
            OVERRIDE_FIND_PACKAGE
            URL https://github.com/Mbed-TLS/mbedtls/releases/download/v2.28.8/mbedtls-2.28.8.tar.bz2
            URL_HASH SHA256=241c68402cef653e586be3ce28d57da24598eb0df13fcdea9d99bfce58717132
    )

    # Prevent warning about empty PROJECT_VERSION vars
    set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)

    # Set flags
    set(USE_STATIC_MBEDTLS_LIBRARY ON CACHE BOOL "Build mbed TLS static library." FORCE)
    set(INSTALL_MBEDTLS_HEADERS OFF CACHE BOOL "Install mbed TLS headers." FORCE)
    set(ENABLE_PROGRAMS OFF CACHE BOOL "Build mbed TLS programs." FORCE)
    set(ENABLE_TESTING OFF CACHE BOOL "Build mbed TLS tests." FORCE)

    # MakeAvailable
    FetchContent_MakeAvailable(mbedtls)

    # Set IDE folders
    set_target_properties(mbedtls PROPERTIES FOLDER "external/mbedtls")
    set_target_properties(mbedcrypto PROPERTIES FOLDER "external/mbedtls")
    set_target_properties(mbedx509 PROPERTIES FOLDER "external/mbedtls")
    set_target_properties(lib PROPERTIES FOLDER "external/mbedtls")
    set_target_properties(apidoc PROPERTIES FOLDER "external/mbedtls")

    if (WIN32 OR ANDROID)
        # cmake find_package defines for mbedtls
        set(MBEDTLS_INCLUDE_DIR mbedtls_SOURCE_DIR CACHE INTERNAL "" FORCE)
        set(MBEDTLS_LIBRARY mbedtls CACHE INTERNAL "" FORCE)
        set(MBEDX509_LIBRARY mbedx509 CACHE INTERNAL "" FORCE)
        set(MBEDCRYPTO_LIBRARY mbedcrypto CACHE INTERNAL "" FORCE)

        mark_as_advanced(MBEDTLS_INCLUDE_DIR)
        mark_as_advanced(MBEDTLS_LIBRARY)
        mark_as_advanced(MBEDX509_LIBRARY)
        mark_as_advanced(MBEDCRYPTO_LIBRARY)
    endif ()
endfunction()

fetch_mbedtls()
