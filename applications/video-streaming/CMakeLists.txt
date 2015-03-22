# cmake configuration for video example

if(OPENSSL_FOUND)

include_directories(${OPENSSL_INCLUDE_DIR})

add_executable (video_publisher video_publisher.cpp)
target_link_libraries (video_publisher blackadder pthread ${OPENSSL_LIBRARIES})

add_executable (video_subscriber video_subscriber.cpp)
target_link_libraries (video_subscriber blackadder pthread ${OPENSSL_LIBRARIES})

install(TARGETS video_publisher video_subscriber DESTINATION bin)

else()

message("openssl is not found - video pub/sub application will not be built")

endif()