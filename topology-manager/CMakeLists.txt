# cmake configuration for topology-manager

if(Boost_FOUND)

  set(TMSRC tm_graph.cpp topology_manager.cpp)
  
  add_executable (topology-manager ${TMSRC})
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries (topology-manager ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} blackadder)
  
  install(TARGETS topology-manager DESTINATION bin)

else()
  message("topology-manager will not be built")
endif()
