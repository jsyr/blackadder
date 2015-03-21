cmake_minimum_required(VERSION 2.8)
if( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE )
message(FATAL_ERROR "In-source builds are not allowed. Create a directory and run cmake from there, passing the path to this source directory.")
endif()

project(topology-manager)

find_package(Boost COMPONENTS program_options graph filesystem system)
if(Boost_FOUND)

  set(TMSRC tm_graph.cpp topology_manager.cpp)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  
  add_executable (topology-manager ${TMSRC})
  target_link_libraries (topology-manager ${Boost_LIBRARIES})
  target_link_libraries (topology-manager blackadder)
  
  install(TARGETS topology-manager DESTINATION bin)
  include_directories(${Boost_INCLUDE_DIRS})

else()
  message(FATAL_ERROR "CMake topology-manager: the following Boost libraries must be installed: program_options, graph, filesystem, system")
endif()
