cmake_minimum_required(VERSION 3.10)
project(chif_net C)

set(CMAKE_C_STANDARD 99)
if (CMAKE_COMPILER_IS_GNUCC)
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
endif ()
if (MSVC)
  set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /W4")
endif ()

include_directories(chif_net)
include_directories(SYSTEM tests/thirdparty)

set(CHIF_NET_SRC
  chif_net/chif_net.c
  chif_net/chif_net.h
  )

set(TESTS_SRC
  tests/thirdparty/alf_test.h
  tests/thirdparty/alf_test.c
  tests/thirdparty/alf_thread.h
  tests/thirdparty/alf_thread.c

  tests/tests.h
  tests/connect.test.c
  tests/echo.test.c
  examples/echo.c
  examples/echo.h
)

add_executable(echo_server examples/echo_server_example.c
  examples/echo.c examples/echo.h ${CHIF_NET_SRC})
target_compile_definitions(echo_server PRIVATE ECHO_DEBUG_PRINT)

add_executable(echo_client examples/echo_client_example.c
  examples/echo.c examples/echo.h ${CHIF_NET_SRC})
target_compile_definitions(echo_client PRIVATE ECHO_DEBUG_PRINT)

add_executable(port_scanner examples/port_scanner.c ${CHIF_NET_SRC})
add_executable(find_ip examples/find_ip.c ${CHIF_NET_SRC})
add_executable(tests tests/test.c ${CHIF_NET_SRC} ${TESTS_SRC})

if (WIN32)
  target_link_libraries(echo_server ws2_32)
  target_link_libraries(echo_client ws2_32)
  target_link_libraries(port_scanner ws2_32)
  target_link_libraries(find_ip ws2_32)
  target_link_libraries(tests ws2_32)
endif ()
if (CMAKE_COMPILER_IS_GNUCC)
  target_link_libraries(tests pthread)
endif ()
