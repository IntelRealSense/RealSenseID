function(add_mbedtls_subdirectory)
	set(USE_STATIC_MBEDTLS_LIBRARY ON CACHE BOOL "Build mbed TLS static library.")
	set(INSTALL_MBEDTLS_HEADERS OFF CACHE BOOL "Install mbed TLS headers.")
	set(ENABLE_PROGRAMS OFF CACHE BOOL "Build mbed TLS programs.")
	set(ENABLE_TESTING OFF CACHE BOOL "Build mbed TLS tests.")

	# prevent warning about empty PROJECT_VERSION vars
	set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)
	add_subdirectory("${THIRD_PARTY_DIRECTORY}/mbedtls-2.28.7")

	add_library(mbedtls::mbedtls ALIAS mbedtls)

	set_target_properties(mbedtls PROPERTIES FOLDER "external/mbedtls")
	set_target_properties(mbedcrypto PROPERTIES FOLDER "external/mbedtls")
	set_target_properties(mbedx509 PROPERTIES FOLDER "external/mbedtls")
	set_target_properties(lib PROPERTIES FOLDER "external/mbedtls")
	set_target_properties(apidoc PROPERTIES FOLDER "external/mbedtls")

	if(UNIX)
		target_compile_options(mbedtls PRIVATE "-Wno-stringop-overflow")
	endif()
endfunction()

add_mbedtls_subdirectory()
