cmake_minimum_required(VERSION 3.10)
project(chif_net C)

set(CMAKE_C_STANDARD 99)

include_directories(chif_net)

add_executable(echo_server examples/echo_server.c chif_net/chif_net.c)
add_executable(ehco_client examples/echo_client.c chif_net/chif_net.c)