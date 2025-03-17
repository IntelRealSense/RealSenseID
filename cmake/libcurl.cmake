if (WIN32)
    include(FetchContent REQUIRED)

    find_package(CURL QUIET NO_DEFAULT_PATH HINTS ${CMAKE_BINARY_DIR}/libcurl)

    if(NOT CURL_FOUND)
        message(STATUS "Building libcurl")

        FetchContent_Declare(
                libcurl
                URL https://github.com/curl/curl/releases/download/curl-8_5_0/curl-8.5.0.tar.xz
                URL_HASH  SHA256=42ab8db9e20d8290a3b633e7fbb3cec15db34df65fd1015ef8ac1e4723750eeb
        )

        FetchContent_GetProperties(libcurl)
        if(NOT libcurl_POPULATED)
            message(STATUS "Fetching libcurl")
            set(FETCHCONTENT_QUIET NO)
            FetchContent_Populate(libcurl)

            message(STATUS "Configuring libcurl in  ${libcurl_BINARY_DIR}")
            execute_process(
                    COMMAND ${CMAKE_COMMAND}
                    -DBUILD_CURL_EXE:BOOL=OFF
                    -DBUILD_SHARED_LIBS:BOOL=OFF
                    -DCURL_USE_SCHANNEL=ON
                    -DCURL_WINDOWS_SSPI=ON
                    -DHTTP_ONLY=ON
                    -DCURL_DISABLE_NTLM=ON
                    -DCURL_DISABLE_KERBEROS_AUTH=ON
                    -DCURL_USE_LIBSSH2=OFF
                    -DCURL_USE_LIBPSL=OFF
                    -DCURL_USE_GSSAPI=OFF
                    -DCURL_ZLIB:BOOL=OFF
                    -DCURL_BROTLI=OFF
                    -DCURL_ZSTD=OFF
                    -DENABLE_IPV6=OFF
                    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/libcurl
                    ${libcurl_SOURCE_DIR}
                    WORKING_DIRECTORY ${libcurl_BINARY_DIR}
            )

            message(STATUS "Building / installing libcurl")
            file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/libcurl)
            execute_process(
                    COMMAND ${CMAKE_COMMAND} --build . --target install --config Release --parallel
                    WORKING_DIRECTORY ${libcurl_BINARY_DIR}
            )
        endif()
    endif()

    find_package(CURL REQUIRED NO_DEFAULT_PATH HINTS ${CMAKE_BINARY_DIR}/libcurl)
else()
    find_package(CURL QUIET)
    if(CURL_FOUND)
    else()
        message(FATAL_ERROR "Could not find CURL. On Debian/Ubuntu, you need to do the following:
        > sudo apt-get update   # First: Update package repository

        # Then install one of the following packages (only one):
        > sudo apt-get install libcurl4-openssl-dev     # Most likely choice
        or
        > sudo apt-get install libcurl4-gnutls-dev
        or
        > sudo apt-get install libcurl4-nss-dev
        .")
    endif()
endif(WIN32)
