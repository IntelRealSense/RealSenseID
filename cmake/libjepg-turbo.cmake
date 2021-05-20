set(ENABLE_SHARED OFF CACHE BOOL "ENABLE SHARED JPEG TURBO")
set(WITH_TURBOJPEG OFF CACHE BOOL "ENABLE TURBOJPEG API")
set(WITH_CRT_DLL ON CACHE BOOL "ENABLE CRT DLL")
set(WITH_SIMD OFF CACHE BOOL "ENABLE SIMD")

# prevent warning about empty PROJECT_VERSION vars
set(CMAKE_POLICY_DEFAULT_CMP0048 NEW)

add_subdirectory(${THIRD_PARTY_DIRECTORY}/libjpeg-turbo_2_1_0 EXCLUDE_FROM_ALL)
set_target_properties(jpeg-static PROPERTIES FOLDER "external/libjpeg-turbo")
set_target_properties(simd PROPERTIES FOLDER "external/libjpeg-turbo")
