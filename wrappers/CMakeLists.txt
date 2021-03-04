if(RSID_C_CLIENT)
    add_subdirectory(c)
endif()

if(WIN32)
    add_subdirectory(csharp)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    add_subdirectory("android/src/main")
endif()
