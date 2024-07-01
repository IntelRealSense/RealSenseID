set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(HEADERS "${SRC_DIR}/StreamConverter.h" "${SRC_DIR}/MetadataDefines.h" "${SRC_DIR}/RawHelper.h")
set(SOURCES "${SRC_DIR}/StreamConverter.cc" "${SRC_DIR}/RawHelper.cc")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	list(APPEND HEADERS "${SRC_DIR}/LinuxCapture.h")
	list(APPEND SOURCES "${SRC_DIR}/LinuxCapture.cc")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	list(APPEND HEADERS "${SRC_DIR}/MSMFCapture.h")
	list(APPEND SOURCES "${SRC_DIR}/MSMFCapture.cc")
endif()

target_sources(${LIBRSID_CPP_TARGET} PRIVATE ${HEADERS} ${SOURCES})
target_include_directories(${LIBRSID_CPP_TARGET} PRIVATE "${SRC_DIR}")