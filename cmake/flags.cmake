function(set_common_compile_opts target)
	set(OPTS)
	if(RSID_PEDANTIC)				
		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
			list(APPEND OPTS "-Wall" "-Wextra" "-Wconversion" "-pedantic" "-Wno-unused-variable")
		elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			list(APPEND OPTS "/W4")
		else()
			message(WARNING "Cannot add extra compiler flags. Unsupported compiler detected: ${CMAKE_CXX_COMPILER_ID}")
		endif()
		
	endif()
		
	if(RSID_PROTECT_STACK)
		if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
			list(APPEND OPTS "-fstack-protector-strong")
		elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			list(APPEND OPTS "/GS") # Guard stack
		else()
			message(WARNING "Cannot add stack protection compiler flags. Unsupported compiler detected: ${CMAKE_CXX_COMPILER_ID}")
		endif()				
	endif()
	
	if(OPTS)
		target_compile_options(${target} PRIVATE ${OPTS})
		message(STATUS "${target} extra flags: ${OPTS}")
	endif()
endfunction()

