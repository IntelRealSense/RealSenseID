cmake_minimum_required(VERSION 3.5.1)

project(RealSenseIDSwigJNI)

set(SWIG_I_FILE "swig.i")

set(JAVA_GEN_PACKAGE "com.intel.realsenseid.api")
string(REPLACE "." "/" JAVA_GEN_SUBDIR ${JAVA_GEN_PACKAGE})
set(CPP_GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cpp_gen)
set(JAVA_GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/java/${JAVA_GEN_SUBDIR})

# Fail the build if SWIG is not found - Ensure SWIG is installed and on the system path
find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})
# Remove old generated files, in case we don't need to generate some of them anymore
file(REMOVE_RECURSE ${JAVA_GEN_DIR})
file(REMOVE_RECURSE ${CPP_GEN_DIR})
file(REMOVE ${SWIG_I_FILE})

# Preprocess the swig i file
set (RealSenseID_HEADERS_FOLDER ${CMAKE_SOURCE_DIR}/include/RealSenseID)
set (API_SIMPLIFIERS_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/api_simplifiers/include)
configure_file(${SWIG_I_FILE}.in ${CMAKE_CURRENT_SOURCE_DIR}/${SWIG_I_FILE})

# Ensure file recognized as C++ (otherwise, exported as C file)
set_property(SOURCE ${SWIG_I_FILE} PROPERTY CPLUSPLUS ON)

# Setup SWIG flags and locations
set(CMAKE_SWIG_FLAGS -package ${JAVA_GEN_PACKAGE} -doxygen)
set(CMAKE_SWIG_OUTDIR ${JAVA_GEN_DIR})
set(SWIG_OUTFILE_DIR ${CPP_GEN_DIR})

# SHOULD NOT BE USED IN PRODUCTION - Not part of the RealSenseID API - Used to create the Android example using rsid_signature_example.
set(SIG_EXAMPLE_I_FILE "signature_example_wrapper.i")
file(REMOVE ${SIG_EXAMPLE_I_FILE})
set (RealSenseID_EXAMPLES_SHARED_FOLDER ${CMAKE_SOURCE_DIR}/examples/shared)
configure_file(${SIG_EXAMPLE_I_FILE}.in ${CMAKE_CURRENT_SOURCE_DIR}/${SIG_EXAMPLE_I_FILE})
set_property(SOURCE ${SIG_EXAMPLE_I_FILE} PROPERTY CPLUSPLUS ON)

# Export a wrapper file to Java, and link with the created C++ library
swig_add_library(${PROJECT_NAME} TYPE MODULE 
	LANGUAGE java
	SOURCES ${SWIG_I_FILE} ${SIG_EXAMPLE_I_FILE}
)

find_library(ANDROID_LIB android)
find_library(ANDROID_LOG log)
swig_link_libraries(${PROJECT_NAME} 
	${ANDROID_LIB}
	${ANDROID_LOG}
	${LIBRSID_CPP_TARGET}
	rsid_signature_example
)
