add_subdirectory(c)

if(WIN32)
    add_subdirectory(csharp)
endif()

if(RSID_PY)
    add_subdirectory(python)
endif()
