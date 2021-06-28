set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(HEADERS "${SRC_DIR}/Matcher.h" "${SRC_DIR}/MatcherImplDefines.h")
set(SOURCES "${SRC_DIR}/Matcher.cc")

if(DEFINED LIBRSID_CPP_TARGET)
    target_sources(${LIBRSID_CPP_TARGET} PRIVATE ${HEADERS} ${SOURCES})
    target_include_directories(${LIBRSID_CPP_TARGET} PRIVATE "${SRC_DIR}")
endif()
