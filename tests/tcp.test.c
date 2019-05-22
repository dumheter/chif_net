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

#include "tests.h"
#include "util.h"
#include <chif_net.h>
#include <stdlib.h>
#include <string.h>

void
tcp_test(AlfTestState* state)
{
  chif_net_socket sock;
  const chif_net_address_family af = CHIF_NET_ADDRESS_FAMILY_IPV4;
  const chif_net_transport_protocol proto = CHIF_NET_TRANSPORT_PROTOCOL_TCP;
  OK_OR_RET(chif_net_open_socket(&sock, proto, af));
  OK_OR_RET(chif_net_set_reuse_addr(sock, CHIF_NET_TRUE));

  const char* portstr = "1336\0";
  chif_net_address addr;
  OK_OR_RET(
    chif_net_create_address(&addr, CHIF_NET_ANY_ADDRESS, portstr, af, proto));

  OK_OR_RET(chif_net_bind(sock, (chif_net_address*)&addr));

  chif_net_port bound_port;
  OK_OR_RET(CHIF_NET_RESULT_SUCCESS ==
            chif_net_port_from_socket(sock, &bound_port));
  char bound_ip[CHIF_NET_IPVX_STRING_LENGTH];
  OK_OR_RET(
    CHIF_NET_RESULT_SUCCESS ==
    chif_net_ip_from_socket(sock, bound_ip, CHIF_NET_IPVX_STRING_LENGTH));
  ALF_CHECK_TRUE(state, bound_port == 1336);
  const char* correct_ip = "0.0.0.0";
  ALF_CHECK_TRUE(state, 0 == memcmp(bound_ip, correct_ip, 7));

  OK_OR_RET(CHIF_NET_RESULT_SUCCESS ==
            chif_net_listen(sock, CHIF_NET_DEFAULT_BACKLOG));

  /* printf("waiting to accept client\n"); */
  /* chif_net_socket clisock; */
  /* chif_net_any_address cliaddr; */
  /* cliaddr.ipv4_address.address_family = af; */
  /* OK_OR_RET( */
  /*     */
  /*     chif_net_accept(sock, (chif_net_address*)&cliaddr, &clisock)); */

  /* char cliip[CHIF_NET_IPVX_STRING_LENGTH]; */
  /* chif_net_port cliport; */
  /* OK_OR_RET( */
  /*     */
  /*     chif_net_ip_from_address((chif_net_address*)&cliaddr, cliip,
   * CHIF_NET_IPVX_STRING_LENGTH)); */
  /* OK_OR_RET( */
  /*     chif_net_port_from_address((chif_net_address*)&cliaddr, &cliport)); */
  /* printf("client connected from %s:%d\n\n", cliip, cliport); */

  /* enum */
  /* { */
  /*   bufsize = 1024 */
  /* }; */
  /* uint8_t buf[bufsize]; */
  /* int bytes; */
  /* while (chif_net_read(clisock, buf, bufsize, &bytes) == */
  /*        CHIF_NET_RESULT_SUCCESS) { */
  /*   printf("read [%s], echoing it back.\n", (char*)buf); */
  /*   chif_net_write(clisock, buf, (size_t)bytes, &bytes); */
  /* } */

  /* printf("closing sockets\n"); */
  /* chif_net_close_socket(&clisock); */
  chif_net_close_socket(&sock);
}
