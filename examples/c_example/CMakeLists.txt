cmake_minimum_required(VERSION 3.10.2)
project(LibRealSenseID_C_Example C)

set(EXE_NAME rsid_c_example)

add_executable(${EXE_NAME} main.c)
target_link_libraries(${EXE_NAME} PRIVATE rsid_c)
if(RSID_SECURE)
    target_link_libraries(${EXE_NAME} PRIVATE rsid_signature_example)
endif()

# set debugger cwd to the exe folder (msvc only)
set_property(TARGET ${EXE_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${EXE_NAME}>")

set_target_properties(${EXE_NAME}
    PROPERTIES 
        COMPILE_DEFINITIONS $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
        FOLDER "examples"
)

# if preview enabled, copy opencv shared libs to the exe folder
if(RSID_PREVIEW)
    target_compile_definitions(${EXE_NAME} PRIVATE "RSID_PREVIEW")
endif()

install(TARGETS ${EXE_NAME})
