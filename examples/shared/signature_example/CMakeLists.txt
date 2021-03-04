# Helper example library that implements signature/verify callbacks

project(RealSenseID_Signature_Example CXX)

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/rsid_signature_example.cc rsid_signature_example.h
            rsid_signature_example_export.h)

set(TARGET_NAME rsid_signature_example)

add_library(${TARGET_NAME} SHARED ${SOURCES})
target_include_directories(${TARGET_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${TARGET_NAME} PUBLIC rsid rsid_c mbedtls::mbedtls)
set_target_properties(${TARGET_NAME} PROPERTIES DEBUG_POSTFIX ${RSID_DEBUG_POSTFIX})

# set ide source group
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "examples")
get_target_property(PROJECT_SOURCES ${TARGET_NAME} SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${PROJECT_SOURCES})
