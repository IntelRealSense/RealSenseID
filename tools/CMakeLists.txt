cmake_minimum_required(VERSION 3.10.2)

if(RSID_SECURE)
	add_subdirectory(shared/secure-mode-helper)
endif()

add_subdirectory(rsid-fw-update)
add_subdirectory(rsid-cli)

if(MSVC)
    add_subdirectory(rsid-viewer)
endif()
