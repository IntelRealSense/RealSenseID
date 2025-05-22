
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})


if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	list(APPEND SOURCES  "${SRC_DIR}/DiscoverDevices_Linux.cc")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	list(APPEND SOURCES  "${SRC_DIR}/DiscoverDevices_Win32.cc")
endif()	

target_sources(${LIBRSID_CPP_TARGET} PRIVATE ${SOURCES})