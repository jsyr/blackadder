cmake_minimum_required(VERSION 2.8)
if( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE )
message(FATAL_ERROR "In-source builds are not allowed. Create a directory and run cmake from there, passing the path to this source directory.")
endif()

project(blackadder-lib)

set( LIBSRC blackadder.cpp nb_blackadder.cpp bitvector.cpp ) 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC"  )

add_library (core OBJECT ${LIBSRC} )
add_library (blackadder STATIC $<TARGET_OBJECTS:core>)

install(TARGETS blackadder DESTINATION lib)

install(FILES blackadder.h nb_blackadder.h bitvector.h blackadder_defs.h DESTINATION include)
