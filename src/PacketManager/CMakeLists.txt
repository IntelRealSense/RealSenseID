set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(HEADERS "${SRC_DIR}/PacketSender.h" "${SRC_DIR}/SerialPacket.h" "${SRC_DIR}/Timer.h"
            "${SRC_DIR}/SerialConnection.h" "${SRC_DIR}/CommonTypes.h")

set(SOURCES "${SRC_DIR}/PacketSender.cc" "${SRC_DIR}/Randomizer.cc" "${SRC_DIR}/SerialPacket.cc" "${SRC_DIR}/Timer.cc")

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    list(APPEND HEADERS "${SRC_DIR}/LinuxSerial.h")
    list(APPEND SOURCES "${SRC_DIR}/LinuxSerial.cc")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    list(APPEND HEADERS "${SRC_DIR}/WindowsSerial.h")
    list(APPEND SOURCES "${SRC_DIR}/WindowsSerial.cc")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    list(APPEND HEADERS
        "${SRC_DIR}/AndroidSerial.h"
        "${SRC_DIR}/CyclicBuffer.h"
    )
    list(APPEND SOURCES
        "${SRC_DIR}/AndroidSerial.cc"
        "${SRC_DIR}/CyclicBuffer.cc"
    )
endif()

list(APPEND HEADERS ${SRC_DIR}/MbedtlsWrapper.h ${SRC_DIR}/SecureHostSession.h)
list(APPEND SOURCES ${SRC_DIR}/MbedtlsWrapper.cc ${SRC_DIR}/SecureHostSession.cc)
    
target_sources(${LIBRSID_CPP_TARGET} PRIVATE ${HEADERS} ${SOURCES})
target_include_directories(${LIBRSID_CPP_TARGET} PRIVATE "${SRC_DIR}")
