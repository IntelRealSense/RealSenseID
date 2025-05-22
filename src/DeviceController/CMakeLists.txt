set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(HEADERS "${SRC_DIR}/DeviceControllerImpl.h")
set(SOURCES
    "${SRC_DIR}/DeviceControllerApi.cc"
    "${SRC_DIR}/DeviceControllerImpl.cc"	
)

target_sources(${LIBRSID_CPP_TARGET} PRIVATE ${HEADERS} ${SOURCES})
