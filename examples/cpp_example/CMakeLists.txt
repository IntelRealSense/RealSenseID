cmake_minimum_required(VERSION 3.13.0)
project(RealSenseID_CPP_Example CXX)

set(EXE_NAME rsid_cpp_example)
add_executable(${EXE_NAME} main.cc)
target_link_libraries(${EXE_NAME} PRIVATE rsid rsid_signature_example)

# set debugger cwd to the exe folder (msvc only)
set_property(TARGET ${EXE_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${EXE_NAME}>")

# if preview enabled, copy opencv shared libs to the exe folder
if(RSID_PREVIEW)
    target_compile_definitions(${EXE_NAME} PRIVATE "RSID_PREVIEW")
endif()

set_target_properties(${EXE_NAME} PROPERTIES FOLDER "examples")
