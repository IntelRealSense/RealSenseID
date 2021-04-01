add_subdirectory(c)


if(WIN32)
    add_subdirectory(csharp)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    add_subdirectory("android/src")
endif()
