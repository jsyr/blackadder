# root cmake configuration file

# project blackadder
project(blackadder)

cmake_minimum_required(VERSION 2.8)

include_directories(lib)
# do not allow in-source builds
if( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE )
message(FATAL_ERROR "In-source builds are not allowed. Create a directory and run cmake from there, passing the path to this source directory.")
endif()

find_package(Boost COMPONENTS program_options graph filesystem system)
find_package(OpenSSL)
find_package(Threads)

# required for other targets to find the library 
# set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /opt/local/lib)

add_subdirectory (lib)
add_subdirectory (deployment)
add_subdirectory (topology-manager)
add_subdirectory (applications)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
# set_property(TARGET blackadder PROPERTY POSITION_INDEPENDENT_CODE ON)
# add an uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
