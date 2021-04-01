include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

message(STATUS "Generating install..")
set(project_config_in "${CMAKE_CURRENT_LIST_DIR}/rsidConfig.cmake.in")
set(project_config_out "${CMAKE_CURRENT_BINARY_DIR}/rsidConfig.cmake")
set(config_targets_file "rsidConfigTargets.cmake")
set(version_config_file "${CMAKE_CURRENT_BINARY_DIR}/rsidConfigVersion.cmake")
set(export_dest_dir "${CMAKE_INSTALL_LIBDIR}/cmake/rsid")


# Install include files
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(DIRECTORY wrappers/c/include/rsid_c DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/RealSenseID")


# Install tools
if(RSID_TOOLS)
    install(TARGETS rsid-cli rsid-fw-update)
    if(RSID_SECURE)
        install(TARGETS rsid_secure_helper)
    endif()
endif()


install(
    TARGETS ${LIBRSID_CPP_TARGET}  ${LIBRSID_C_TARGET} 
    EXPORT rsid LIBRARY ARCHIVE RUNTIME)
    
    
# Install CMake config files
install(EXPORT rsid DESTINATION ${export_dest_dir} NAMESPACE rsid:: FILE ${config_targets_file})
configure_file("${project_config_in}" "${project_config_out}" @ONLY)
write_basic_package_version_file("${version_config_file}" COMPATIBILITY SameMajorVersion)
install(FILES "${project_config_out}" "${version_config_file}" DESTINATION "${export_dest_dir}")
