if(RSID_SECURE)
	add_subdirectory(shared/signature_example)
endif()

add_subdirectory(cpp_example)

if(RSID_C_CLIENT)
    add_subdirectory(c_example)
endif()

if(WIN32)
    add_subdirectory(csharp_example)
endif()
