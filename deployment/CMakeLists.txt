# cmake configuration for deployment tool

if(Boost_FOUND)

  set(DEPLOYMENT-SRC params.cpp bitvector.cpp network.cpp graph.cpp deploy.cpp)
  
  add_executable (deploy ${DEPLOYMENT-SRC})
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries (deploy ${Boost_LIBRARIES})
  
  install(TARGETS deploy DESTINATION bin)

else()
  message("deployment tool will not be built")
endif()