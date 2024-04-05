include(ExternalProject)

set(LIBJPEG_TURBO_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/libjpeg-turbo-build/${CMAKE_BUILD_TYPE}")

ExternalProject_Add(
        libjpeg-turbo
		PREFIX _deps/libjpeg-turbo
        URL https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.0.2/libjpeg-turbo-3.0.2.tar.gz
        URL_HASH SHA256=c2ce515a78d91b09023773ef2770d6b0df77d674e144de80d63e0389b3a15ca6        
		EXCLUDE_FROM_ALL 		
		 CMAKE_ARGS
			-DENABLE_SHARED=OFF
			-DWITH_TURBOJPEG=OFF
			-DWITH_CRT_DLL=ON
			-DWITH_SIMD=OFF						
			-DCMAKE_POSITION_INDEPENDENT_CODE=ON
			-DCMAKE_INSTALL_PREFIX=${LIBJPEG_TURBO_INSTALL_DIR}
)


set_target_properties(libjpeg-turbo PROPERTIES FOLDER "external/libjpeg-turbo")