cmake_minimum_required(VERSION 2.8)
if( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE )
message(FATAL_ERROR "In-source builds are not allowed. Create a directory and run cmake from there, passing the path to this source directory.")
endif()

project(deploy)

find_package(Boost COMPONENTS program_options graph filesystem system)
if(Boost_FOUND)

  set(DEPLOYMENT-SRC params.cpp bitvector.cpp network.cpp graph.cpp deploy.cpp)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  
  add_executable (deploy ${DEPLOYMENT-SRC})
  target_link_libraries (deploy ${Boost_LIBRARIES})
  
  install(TARGETS deploy DESTINATION bin)
  include_directories(${Boost_INCLUDE_DIRS})

else()
  message(FATAL_ERROR "CMake topology-manager: the following Boost libraries must be installed: program_options, graph, filesystem, system")
endif()
