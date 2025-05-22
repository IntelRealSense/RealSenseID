
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})


target_compile_definitions(${LIBRSID_CPP_TARGET} PRIVATE RSID_PREVIEW)
list(APPEND HEADERS "${SRC_DIR}/PreviewImpl.h")
list(APPEND SOURCES 
	"${SRC_DIR}/PreviewApi.cc"
	"${SRC_DIR}/PreviewImpl.cc"
)
	
target_sources(${LIBRSID_CPP_TARGET} PRIVATE ${SOURCES})

# libjpeg-turbo-install 
if(NOT WIN32)
	add_dependencies(${LIBRSID_CPP_TARGET} libjpeg-turbo) 
	target_include_directories(${LIBRSID_CPP_TARGET} PRIVATE "${LIBJPEG_TURBO_INSTALL_DIR}/include")	
	target_link_directories(${LIBRSID_CPP_TARGET} PRIVATE "${LIBJPEG_TURBO_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}")	
	target_link_libraries(${LIBRSID_CPP_TARGET} PRIVATE LibUVC::UVC jpeg)
endif()	

add_subdirectory("${SRC_DIR}/Capture")
	

