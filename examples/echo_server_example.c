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

#include "util.h"
#include <chif_net.h>
#include <stdlib.h>

// ============================================================ //

int
run_server(int argc, char** argv);

// ============================================================ //

int
main(int argc, char** argv)
{
  chif_net_startup();
  printf("running echo server\n");

  const int ret = run_server(argc, argv);

  printf("exiting\n");
  chif_net_shutdown();

#if defined(_WIN32) || defined(_WIN64)
  printf("\nenter any key to exit\n> ");
  const int in = getchar();
  CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(in);
#endif

  return ret;
}

// ============================================================ //

int
run_server(int argc, char** argv)
{
  // parse settings, if any
  chif_net_address_family af = CHIF_NET_ADDRESS_FAMILY_IPV4;
  chif_net_transport_protocol proto = CHIF_NET_TRANSPORT_PROTOCOL_TCP;
  chif_net_port port = 1337;
  int i = 0;
  while (++i < argc) {
    if ((char)argv[i][0] == '-') {
      switch (argv[i][1]) {
        case 'p': { // PORT
          if (i + 1 < argc) {
            port = (chif_net_port)atoi(argv[i + 1]);
          }
          break;
        }
        case '6': { // IPv6
          af = CHIF_NET_ADDRESS_FAMILY_IPV6;
          break;
        }
        case '4': { // IPv4
          af = CHIF_NET_ADDRESS_FAMILY_IPV4;
          break;
        }
        case 't': { // TCP
          proto = CHIF_NET_TRANSPORT_PROTOCOL_TCP;
          break;
        }
        case 'u': { // UDP
          proto = CHIF_NET_TRANSPORT_PROTOCOL_UDP;
          break;
        }
      }
    }
  }

  printf("open socket with protocol [%s], address family [%s]\n",
         chif_net_transport_protocol_to_string(proto),
         chif_net_address_family_to_string(af));
  chif_net_socket sock;
  OK_OR_CRASH(chif_net_open_socket(&sock, proto, af));

  printf("bind socket on port [%u]\n", port);
  chif_net_address bindaddr;
  OK_OR_CRASH(chif_net_create_address_i(
    &bindaddr, CHIF_NET_ANY_ADDRESS, port, af, proto));
  OK_OR_CRASH(chif_net_bind(sock, &bindaddr));

  chif_net_port bound_port;
  OK_OR_CRASH(chif_net_port_from_socket(sock, &bound_port));
  char bound_ip[CHIF_NET_IPVX_STRING_LENGTH];
  OK_OR_CRASH(
    chif_net_ip_from_socket(sock, bound_ip, CHIF_NET_IPVX_STRING_LENGTH));
  printf("socket bound on [%s:%u]\n", bound_ip, bound_port);

  chif_net_socket clisock = CHIF_NET_INVALID_SOCKET;
  chif_net_address cliaddr;
  if (proto == CHIF_NET_TRANSPORT_PROTOCOL_TCP) {
    printf("listen for connection\n");
    OK_OR_CRASH(chif_net_listen(sock, CHIF_NET_DEFAULT_BACKLOG));

    printf("waiting to accept client\n");
    cliaddr.address_family = CHIF_NET_ADDRESS_FAMILY_IPV4;
    OK_OR_CRASH(chif_net_accept(sock, &cliaddr, &clisock));

    char cliip[CHIF_NET_IPVX_STRING_LENGTH];
    chif_net_port cliport;
    OK_OR_CRASH(
      chif_net_ip_from_address(&cliaddr, cliip, CHIF_NET_IPVX_STRING_LENGTH));
    OK_OR_CRASH(chif_net_port_from_address(&cliaddr, &cliport));
    printf("client connected from %s:%d\n", cliip, cliport);
  }

  printf("waiting for messsage\n");
  enum
  {
    bufsize = 1024
  };
  uint8_t buf[bufsize];
  int bytes;
  if (proto == CHIF_NET_TRANSPORT_PROTOCOL_TCP) {
    while (chif_net_read(clisock, buf, bufsize, &bytes) ==
             CHIF_NET_RESULT_SUCCESS &&
           bytes > 0) {
      printf("read [%s], echoing it back.\n", (char*)buf);
      chif_net_write(clisock, buf, (size_t)bytes, &bytes);
    }
  } else {
    OK_OR_CRASH(chif_net_readfrom(sock, buf, bufsize, &bytes, &cliaddr));

    char srcip[CHIF_NET_IPVX_STRING_LENGTH];
    OK_OR_CRASH(
      chif_net_ip_from_address(&cliaddr, srcip, CHIF_NET_IPVX_STRING_LENGTH));
    chif_net_port srcport;
    OK_OR_CRASH(chif_net_port_from_address(&cliaddr, &srcport));
    printf(
      "read [%s] from [%s:%d], echoing it back.\n", (char*)buf, srcip, srcport);

    OK_OR_CRASH(chif_net_writeto(sock, buf, (size_t)bytes, &bytes, &cliaddr));
  }

  printf("closing sockets\n");
  chif_net_close_socket(&clisock);
  chif_net_close_socket(&sock);

  return 0;
}
