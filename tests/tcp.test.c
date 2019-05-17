/**
 * MIT License
 *
 * Copyright (c) 2019 Christoffer Gustafsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "../examples/echo.h"
#include "tests.h"
#include <chif_net.h>
#include <stdio.h>
#include <stdlib.h>

void
run_echo_server(AlfTestState* state)
{
  chif_net_socket sock;
  const chif_net_address_family af = CHIF_NET_ADDRESS_FAMILY_IPV4;
  const chif_net_transport_protocol proto = CHIF_NET_TRANSPORT_PROTOCOL_TCP;
  ALF_CHECK_TRUE(
    state, CHIF_NET_RESULT_SUCCESS == chif_net_open_socket(&sock, proto, af));

  const chif_net_port port = 1337;
  ALF_CHECK_TRUE(state,
                 CHIF_NET_RESULT_SUCCESS == chif_net_bind(sock, port, af));

  chif_net_port bound_port;
  ALF_CHECK_TRUE(state,
                 CHIF_NET_RESULT_SUCCESS ==
                   chif_net_port_from_socket(sock, &bound_port));
  char bound_ip[CHIF_NET_IPVX_STRING_LENGTH];
  ALF_CHECK_TRUE(
    state,
    CHIF_NET_RESULT_SUCCESS ==
      chif_net_ip_from_socket(sock, bound_ip, CHIF_NET_IPVX_STRING_LENGTH));
  printf("socket bound on %s:%u\n", bound_ip, bound_port);

  printf("listen for connection\n");
  ALF_CHECK_TRUE(state,
                 CHIF_NET_RESULT_SUCCESS ==
                   chif_net_listen(sock, CHIF_NET_DEFAULT_BACKLOG));

  printf("waiting to accept client\n");
  chif_net_socket clisock;
  chif_net_address cliaddr;
  ALF_CHECK_TRUE(state,
                 CHIF_NET_RESULT_SUCCESS ==
                   chif_net_accept(sock, &cliaddr, &clisock));

  char cliip[CHIF_NET_IPVX_STRING_LENGTH];
  chif_net_port cliport;
  ALF_CHECK_TRUE(
    state,
    CHIF_NET_RESULT_SUCCESS ==
      chif_net_ip_from_address(&cliaddr, cliip, CHIF_NET_IPVX_STRING_LENGTH));
  ALF_CHECK_TRUE(state,
                 CHIF_NET_RESULT_SUCCESS ==
                   chif_net_port_from_address(&cliaddr, &cliport));
  printf("client connected from %s:%d\n\n", cliip, cliport);

  enum
  {
    bufsize = 1024
  };
  uint8_t buf[bufsize];
  int bytes;
  while (chif_net_read(clisock, buf, bufsize, &bytes) ==
         CHIF_NET_RESULT_SUCCESS) {
    printf("read [%s], echoing it back.\n", (char*)buf);
    chif_net_write(clisock, buf, (size_t)bytes, &bytes);
  }

  printf("closing sockets\n");
  chif_net_close_socket(&clisock);
  chif_net_close_socket(&sock);
}
