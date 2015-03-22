# cmake configuration for fountain-coding sample application

if(OPENSSL_FOUND)

include_directories(${OPENSSL_INCLUDE_DIR})

add_executable (encoder soliton.cpp encoder.cpp main_encoder.cpp prng.cpp)
target_link_libraries (encoder blackadder pthread ${OPENSSL_LIBRARIES})
  
add_executable (fast_encoder soliton.cpp encoder.cpp fast_main_encoder.cpp prng.cpp)
target_link_libraries (fast_encoder blackadder pthread ${OPENSSL_LIBRARIES})
  
add_executable (decoder soliton.cpp decoder.cpp main_decoder.cpp prng.cpp)
target_link_libraries (decoder blackadder pthread ${OPENSSL_LIBRARIES})
  
else()

message("openssl is not found - fountain-coding sample application will not be built")

endif()