function(add_restclient_subdirectory)
	set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
	set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "")
	set(COMPILE_TYPE STATIC CACHE STRING INTERNAL "")
	add_subdirectory(${THIRD_PARTY_DIRECTORY}/restclient-cpp-0.5.2-fork EXCLUDE_FROM_ALL)
	set_target_properties(restclient-cpp PROPERTIES FOLDER "external/restclient-cpp")
endfunction()

add_restclient_subdirectory()