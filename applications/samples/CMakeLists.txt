# cmake configuration for fountain-coding samples

add_executable (channel_publisher channel_publisher.cpp)
target_link_libraries (channel_publisher blackadder pthread)
  
add_executable (channel_subscriber channel_subscriber.cpp)
target_link_libraries (channel_subscriber blackadder pthread)
  
add_executable (nb_channel_publisher nb_channel_publisher.cpp)
target_link_libraries (nb_channel_publisher blackadder pthread)
  
add_executable (nb_channel_subscriber nb_channel_subscriber.cpp)
target_link_libraries (nb_channel_subscriber blackadder pthread)
  
add_executable (publisher publisher.cpp)
target_link_libraries (publisher blackadder pthread)
  
add_executable (subscriber subscriber.cpp)
target_link_libraries (subscriber blackadder pthread)

add_executable (nb_publisher nb_publisher.cpp)
target_link_libraries (nb_publisher blackadder pthread)  
  
add_executable (nb_subscriber nb_subscriber.cpp)
target_link_libraries (nb_subscriber blackadder pthread)
  
add_executable (broadcast_publisher broadcast_publisher.cpp)
target_link_libraries (broadcast_publisher blackadder pthread)
  
add_executable (broadcast_subscriber broadcast_subscriber.cpp)
target_link_libraries (broadcast_subscriber blackadder pthread)
  
add_executable (link_local_publisher link_local_publisher.cpp)
target_link_libraries (link_local_publisher blackadder pthread)
  
add_executable (link_local_subscriber link_local_subscriber.cpp)
target_link_libraries (link_local_subscriber blackadder pthread)
  
add_executable (implicit_rendezvous_publisher implicit_rendezvous_publisher.cpp)
target_link_libraries (implicit_rendezvous_publisher blackadder pthread)
  
add_executable (implicit_rendezvous_subscriber implicit_rendezvous_subscriber.cpp)
target_link_libraries (implicit_rendezvous_subscriber blackadder pthread)
  
add_executable (implicit_algid_rendezvous_publisher implicit_algid_rendezvous_publisher.cpp)
target_link_libraries (implicit_algid_rendezvous_publisher blackadder pthread)
  
add_executable (implicit_algid_rendezvous_subscriber implicit_algid_rendezvous_subscriber.cpp)
target_link_libraries (implicit_algid_rendezvous_subscriber blackadder pthread)
  
install(TARGETS channel_subscriber channel_subscriber nb_channel_publisher nb_channel_subscriber DESTINATION bin)

install(TARGETS publisher subscriber nb_publisher nb_subscriber DESTINATION bin)
  
install(TARGETS broadcast_publisher broadcast_subscriber link_local_publisher link_local_subscriber DESTINATION bin)
 
install(TARGETS implicit_rendezvous_publisher implicit_rendezvous_subscriber implicit_algid_rendezvous_publisher implicit_algid_rendezvous_subscriber DESTINATION bin)

if(OPENSSL_FOUND)

include_directories(${OPENSSL_INCLUDE_DIR})

add_executable (algid_publisher algid_publisher.cpp)
target_link_libraries (algid_publisher blackadder pthread ${OPENSSL_LIBRARIES})
  
add_executable (algid_subscriber algid_subscriber.cpp)
target_link_libraries (algid_subscriber blackadder pthread ${OPENSSL_LIBRARIES})

install(TARGETS algid_publisher algid_subscriber DESTINATION bin)

else()

message("openssl is not found - algid pub/sub sample application will not be built")

endif()