target_sources(${LIBRSID_CPP_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Logger.h")
target_include_directories(${LIBRSID_CPP_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

if(RSID_DEBUG_MODE)
    target_sources(${LIBRSID_CPP_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Logger.cc")
    target_compile_definitions(${LIBRSID_CPP_TARGET} PRIVATE RSID_DEBUG_MODE)
    target_link_libraries(${LIBRSID_CPP_TARGET} PRIVATE spdlog::spdlog)

    if(RSID_DEBUG_SERIAL)
        target_compile_definitions(${LIBRSID_CPP_TARGET} PRIVATE RSID_DEBUG_SERIAL)
    endif()
endif()
