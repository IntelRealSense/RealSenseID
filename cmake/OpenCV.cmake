find_package(OpenCV 4.3.0 REQUIRED)
add_library(opencv INTERFACE)
target_link_libraries(opencv INTERFACE ${OpenCV_LIBS})
target_include_directories(opencv INTERFACE ${OpenCV_INCLUDE_DIRS})

# copy required opencv libs that required for preview: opencv_core opencv_imgcodecs opencv_imgproc opencv_videoio

function(target_copy_opencv_files target_name)
    set(opencv_glob "${OpenCV_INSTALL_PATH}/bin/**/*${CMAKE_SHARED_LIBRARY_SUFFIX}")
    file(GLOB OPENCV_FILES "${opencv_glob}")

    foreach(OPENCV_FILE IN LISTS OPENCV_FILES)
        if(OPENCV_FILE MATCHES "core|imgcodecs|imgproc|videoio")
            if(NOT "${OPENCV_FILE}" MATCHES "ffmpeg") # dont copy videoio_ffmpeg
                message(STATUS "Post build copy ${OPENCV_FILE} for target ${target_name}")
                add_custom_command(
                    TARGET ${target_name} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OPENCV_FILE}"
                                                             "$<TARGET_FILE_DIR:${target_name}>")
            endif()
        endif()
    endforeach()
endfunction()
