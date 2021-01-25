set(USE_STATIC_MBEDTLS_LIBRARY ON)
set(INSTALL_MBEDTLS_HEADERS OFF)
set(ENABLE_PROGRAMS OFF)
set(ENABLE_TESTING OFF)
set(INSTALL_MBEDTLS_HEADERS OFF)

add_subdirectory("${THIRD_PARTY_DIRECTORY}/mbedtls-2.24.0")

add_library(mbedtls::mbedtls ALIAS mbedtls)

set_target_properties(mbedtls PROPERTIES FOLDER "mbedtls")
set_target_properties(mbedcrypto PROPERTIES FOLDER "mbedtls")
set_target_properties(mbedx509 PROPERTIES FOLDER "mbedtls")
set_target_properties(lib PROPERTIES FOLDER "mbedtls")
set_target_properties(apidoc PROPERTIES FOLDER "mbedtls")

# TODO: this seems to cause issues on older clang versions.
# suppress string-concatenation warning in clang for mbedcrypto
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(mbedcrypto PRIVATE "-Wno-string-concatenation")
endif()
