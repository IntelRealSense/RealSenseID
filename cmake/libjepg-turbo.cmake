include(ExternalProject)

set(LIBJPEG_TURBO_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/libjpeg-turbo-build/${CMAKE_BUILD_TYPE}")

ExternalProject_Add(
        libjpeg-turbo
		PREFIX _deps/libjpeg-turbo
        URL https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.0.3/libjpeg-turbo-3.0.3.tar.gz
        URL_HASH SHA256=343e789069fc7afbcdfe44dbba7dbbf45afa98a15150e079a38e60e44578865d
		BUILD_COMMAND ${CMAKE_COMMAND} --build . --config Release --parallel
		EXCLUDE_FROM_ALL
		 CMAKE_ARGS
			-DENABLE_SHARED=OFF
			-DWITH_TURBOJPEG=OFF
			-DWITH_CRT_DLL=ON
			-DWITH_SIMD=ON
			-DWITH_JPEG8=ON
			-DCMAKE_POSITION_INDEPENDENT_CODE=ON
			-DCMAKE_INSTALL_PREFIX=${LIBJPEG_TURBO_INSTALL_DIR}
)


set_target_properties(libjpeg-turbo PROPERTIES FOLDER "external/libjpeg-turbo")