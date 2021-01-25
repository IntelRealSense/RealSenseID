target_sources(${LIBRSID_CPP_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Logger.h")
target_include_directories(${LIBRSID_CPP_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_sources(${LIBRSID_CPP_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Logger.cc")   
target_link_libraries(${LIBRSID_CPP_TARGET} PRIVATE spdlog::spdlog)

target_sources(${LIBRSID_CPP_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Logger.cc")    
target_link_libraries(${LIBRSID_CPP_TARGET} PRIVATE spdlog::spdlog)


if(RSID_DEBUG_CONSOLE)
    target_compile_definitions(${LIBRSID_CPP_TARGET} PRIVATE RSID_DEBUG_CONSOLE)
endif()

if(RSID_DEBUG_FILE)
    target_compile_definitions(${LIBRSID_CPP_TARGET} PRIVATE RSID_DEBUG_FILE)
endif()

if(RSID_DEBUG_SERIAL)
    target_compile_definitions(${LIBRSID_CPP_TARGET} PRIVATE RSID_DEBUG_SERIAL)
endif()


