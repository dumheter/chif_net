cmake_minimum_required(VERSION 3.10)
project(chif_net C)
set(CMAKE_C_STANDARD 99)

if (MSVC)
  set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} /W4")
else ()
  set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
endif ()

include_directories(SYSTEM chif_net)

set(CHIF_NET_BUILD_EXTRA true)

if (CHIF_NET_BUILD_EXTRA)
  include_directories(SYSTEM tests/thirdparty)
endif ()

set(CHIF_NET_SRC
  chif_net/chif_net.c
  chif_net/chif_net.h
  )

if (CHIF_NET_BUILD_EXTRA)
set(EXTRA_SRC
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
endif ()

add_library(${PROJECT_NAME} STATIC ${CHIF_NET_SRC})

if (CHIF_NET_BUILD_EXTRA)
  add_executable(echo_server examples/echo_server_example.c examples/echo.c)
  target_compile_definitions(echo_server PRIVATE ECHO_DEBUG_PRINT)
  add_executable(echo_client examples/echo_client_example.c examples/echo.c)
  target_compile_definitions(echo_client PRIVATE ECHO_DEBUG_PRINT)
  add_executable(find_ip examples/find_ip.c)
  add_executable(tests tests/test.c ${CHIF_NET_SRC} ${EXTRA_SRC})
endif ()

if (WIN32)
  target_link_libraries(${PROJECT_NAME} ws2_32)
  if (CHIF_NET_BUILD_EXTRA)
    target_link_libraries(echo_server chif_net ws2_32)
    target_link_libraries(echo_client chif_net ws2_32)
    target_link_libraries(find_ip chif_net ws2_32)
    target_link_libraries(tests chif_net ws2_32)
  endif ()
else ()
  if (CHIF_NET_BUILD_EXTRA)
    target_link_libraries(echo_server chif_net)
    target_link_libraries(echo_client chif_net)
    target_link_libraries(find_ip chif_net)
    target_link_libraries(tests chif_net pthread)
  endif ()
endif ()
