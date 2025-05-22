set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(HEADERS     
    "${SRC_DIR}/Utilities.h"
    "${SRC_DIR}/Cmds.h"
    "${SRC_DIR}/ModuleInfo.h"
    "${SRC_DIR}/FwUpdaterCommF46x.h"
    "${SRC_DIR}/FwUpdateEngineF46x.h"
	"${SRC_DIR}/FwUpdaterF46x.h"
)

set(SOURCES 
    "${SRC_DIR}/Utilities.cc"
    "${SRC_DIR}/Cmds.cc"
    "${SRC_DIR}/FwUpdaterCommF46x.cc"
    "${SRC_DIR}/FwUpdateEngineF46x.cc"
	"${SRC_DIR}/FwUpdaterF46x.cc"
)

target_sources(${LIBRSID_CPP_TARGET} PRIVATE ${HEADERS} ${SOURCES})
