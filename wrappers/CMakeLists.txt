if(NOT ANDROID)
    add_subdirectory(c)
endif()

if(WIN32)
    add_subdirectory(csharp)
endif()

if(ANDROID)
    add_subdirectory("android/realsenseid/src")
endif()

if(RSID_PY)
    add_subdirectory(python)
endif()
