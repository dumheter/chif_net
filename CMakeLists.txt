cmake_minimum_required(VERSION 3.10)
project(chif_net C)

set(CMAKE_C_STANDARD 99)

include_directories(chif_net)

add_executable(echo_server examples/echo_server.c chif_net/chif_net.c chif_net/chif_net.h)
add_executable(echo_client examples/echo_client.c chif_net/chif_net.c chif_net/chif_net.h)

if (WIN32)
  target_link_libraries(echo_server ws2_32)
  target_link_libraries(echo_client ws2_32)
endif ()
