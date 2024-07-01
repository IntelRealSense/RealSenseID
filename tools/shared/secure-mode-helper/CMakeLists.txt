project(RealSenseID_SecureMode_Helper CXX)

set(SOURCES secure_mode_helper.cc secure_mode_helper.h secure_mode_helper_export.h)

set(TARGET_NAME rsid_secure_helper)

add_library(${TARGET_NAME} SHARED ${SOURCES})
target_include_directories(${TARGET_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${TARGET_NAME} PUBLIC rsid rsid_c mbedtls)
set_target_properties(${TARGET_NAME} PROPERTIES DEBUG_POSTFIX ${RSID_DEBUG_POSTFIX})

# set ide source group
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "tools")
get_target_property(PROJECT_SOURCES ${TARGET_NAME} SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${PROJECT_SOURCES})
