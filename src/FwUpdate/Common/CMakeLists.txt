set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(SOURCES       
    "${SRC_DIR}/Common.h" "${SRC_DIR}/Common.cc"
)

target_sources(${LIBRSID_CPP_TARGET} PRIVATE ${SOURCES})
