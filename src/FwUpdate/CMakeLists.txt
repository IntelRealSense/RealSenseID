set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(HEADERS     
    "${SRC_DIR}/Utilities.h"
    "${SRC_DIR}/Cmds.h"
    "${SRC_DIR}/ModuleInfo.h"
    "${SRC_DIR}/FwUpdaterComm.h"
    "${SRC_DIR}/FwUpdateEngine.h"
)

set(SOURCES 
    "${SRC_DIR}/Utilities.cc"
    "${SRC_DIR}/Cmds.cc"
    "${SRC_DIR}/FwUpdaterComm.cc"
    "${SRC_DIR}/FwUpdateEngine.cc"
)

target_sources(${LIBRSID_CPP_TARGET} PRIVATE ${HEADERS} ${SOURCES})
target_include_directories(${LIBRSID_CPP_TARGET} PRIVATE "${SRC_DIR}")
