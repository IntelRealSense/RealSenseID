set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(HEADERS "${SRC_DIR}/StreamConverter.h")
set(SOURCES "${SRC_DIR}/StreamConverter.cc")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	list(APPEND HEADERS "${SRC_DIR}/LinuxCapture.h")
	list(APPEND SOURCES "${SRC_DIR}/LinuxCapture.cc")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	list(APPEND HEADERS "${SRC_DIR}/MSMFCapture.h")
	list(APPEND SOURCES "${SRC_DIR}/MSMFCapture.cc")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	list(APPEND HEADERS "${SRC_DIR}/AndroidCapture.h")
	list(APPEND SOURCES "${SRC_DIR}/AndroidCapture.cc")
endif()

target_sources(${LIBRSID_CPP_TARGET} PRIVATE ${HEADERS} ${SOURCES})
target_include_directories(${LIBRSID_CPP_TARGET} PRIVATE "${SRC_DIR}")