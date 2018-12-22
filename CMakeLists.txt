cmake_minimum_required(VERSION 3.10)
project(chif_net C)

set(CMAKE_C_STANDARD 99)

include_directories(chif_net)

set(CHIF_NET_SRC
  chif_net/chif_net.c
  chif_net/chif_net.h
  )

add_executable(echo_server_tcp examples/echo_server_tcp.c ${CHIF_NET_SRC})
add_executable(echo_client_tcp examples/echo_client_tcp.c ${CHIF_NET_SRC})
add_executable(echo_server_udp examples/echo_server_udp.c ${CHIF_NET_SRC})
add_executable(echo_client_udp examples/echo_client_udp.c ${CHIF_NET_SRC})

if (WIN32)
  target_link_libraries(echo_server_tcp ws2_32)
  target_link_libraries(echo_client_tcp ws2_32)
  target_link_libraries(echo_server_udp ws2_32)
  target_link_libraries(echo_client_udp ws2_32)
endif ()
