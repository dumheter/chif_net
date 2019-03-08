cmake_minimum_required(VERSION 3.10)
project(chif_net C)

set(CMAKE_C_STANDARD 99)
if (CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic  ")
endif ()
if (MSVC)
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /W4")
endif ()


include_directories(chif_net)

set(CHIF_NET_SRC
  chif_net/chif_net.c
  chif_net/chif_net.h
  )

add_executable(echo_server_tcp examples/echo_server_tcp.c ${CHIF_NET_SRC})
add_executable(echo_client_tcp examples/echo_client_tcp.c ${CHIF_NET_SRC})
add_executable(ipv6_echo_server_tcp examples/ipv6_echo_server_tcp.c ${CHIF_NET_SRC})
add_executable(ipv6_echo_client_tcp examples/ipv6_echo_client_tcp.c ${CHIF_NET_SRC})
add_executable(echo_server_udp examples/echo_server_udp.c ${CHIF_NET_SRC})
add_executable(echo_client_udp examples/echo_client_udp.c ${CHIF_NET_SRC})
add_executable(ipv6_echo_server_udp examples/ipv6_echo_server_udp.c ${CHIF_NET_SRC})
add_executable(ipv6_echo_client_udp examples/ipv6_echo_client_udp.c ${CHIF_NET_SRC})
add_executable(port_scanner examples/port_scanner.c ${CHIF_NET_SRC})
add_executable(find_ip examples/find_ip.c ${CHIF_NET_SRC})

if (WIN32)
  target_link_libraries(echo_server_tcp ws2_32)
  target_link_libraries(echo_client_tcp ws2_32)
  target_link_libraries(ipv6_echo_server_tcp ws2_32)
  target_link_libraries(ipv6_echo_client_tcp ws2_32)
  target_link_libraries(echo_server_udp ws2_32)
  target_link_libraries(echo_client_udp ws2_32)
  target_link_libraries(ipv6_echo_server_udp ws2_32)
  target_link_libraries(ipv6_echo_client_udp ws2_32)
  target_link_libraries(port_scanner ws2_32)
  target_link_libraries(find_ip ws2_32)
endif ()
