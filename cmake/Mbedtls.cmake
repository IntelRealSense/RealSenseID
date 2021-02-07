set(USE_STATIC_MBEDTLS_LIBRARY ON CACHE BOOL "Build mbed TLS static library.")
set(INSTALL_MBEDTLS_HEADERS OFF CACHE BOOL "Install mbed TLS headers.")
set(ENABLE_PROGRAMS OFF CACHE BOOL "Build mbed TLS programs.")
set(ENABLE_TESTING OFF CACHE BOOL "Build mbed TLS tests.")

add_subdirectory("${THIRD_PARTY_DIRECTORY}/mbedtls-2.25.0")

add_library(mbedtls::mbedtls ALIAS mbedtls)

set_target_properties(mbedtls PROPERTIES FOLDER "mbedtls")
set_target_properties(mbedcrypto PROPERTIES FOLDER "mbedtls")
set_target_properties(mbedx509 PROPERTIES FOLDER "mbedtls")
set_target_properties(lib PROPERTIES FOLDER "mbedtls")
set_target_properties(apidoc PROPERTIES FOLDER "mbedtls")
