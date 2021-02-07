set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    message(STATUS "Windows platform")
    set(CMAKE_CXX_STANDARD 14)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    message(STATUS "Linux platform")
    set(CMAKE_CXX_STANDARD 14)
    add_definitions(-DLINUX)

elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    message(STATUS "Android platform")
    set(CMAKE_CXX_STANDARD 14)
    add_definitions(-DANDROID)
    add_definitions(-DANDROID_STL=c++_shared)
    
else()
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} is currently unsupported")
endif()
