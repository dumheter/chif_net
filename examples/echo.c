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

#include "echo.h"
#include <chif_net.h>
#include <stdio.h>
#include <stdlib.h>

#define OK_OR_DIE(fn)                                                          \
  {                                                                            \
    const chif_net_result res = (fn);                                          \
    if (res != CHIF_NET_RESULT_SUCCESS) {                                      \
      DEBUG_PRINTF("failed with error [%s]\n",                                 \
                   chif_net_result_to_string(res));                            \
      return -1;                                                               \
    }                                                                   \
}

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

  DEBUG_PRINTF("open socket with protocol [%s], address family [%s]\n",
               chif_net_transport_protocol_to_string(proto),
               chif_net_address_family_to_string(af));
  chif_net_socket sock;
  OK_OR_DIE(chif_net_open_socket(&sock, proto, af));

  enum
  {
	  portstrlen = 6
  };
  char portstr[portstrlen];
  snprintf(portstr, portstrlen, "%u%c", port, '\0');
  DEBUG_PRINTF("bind socket on port [%s]\n", portstr);
  chif_net_any_address bindaddr;

  OK_OR_DIE(chif_net_create_address(
    (chif_net_address*)&bindaddr, CHIF_NET_ANY_ADDRESS, portstr, af, proto));
  OK_OR_DIE(chif_net_bind(sock, (chif_net_address*)&bindaddr));

  chif_net_port bound_port;
  OK_OR_DIE(chif_net_port_from_socket(sock, &bound_port));
  char bound_ip[CHIF_NET_IPVX_STRING_LENGTH];
  OK_OR_DIE(
    chif_net_ip_from_socket(sock, bound_ip, CHIF_NET_IPVX_STRING_LENGTH));
  DEBUG_PRINTF("socket bound on [%s:%u]\n", bound_ip, bound_port);

  chif_net_socket clisock = CHIF_NET_INVALID_SOCKET;
  chif_net_address cliaddr;
  if (proto == CHIF_NET_TRANSPORT_PROTOCOL_TCP) {
    DEBUG_PRINTF("listen for connection\n");
    OK_OR_DIE(chif_net_listen(sock, CHIF_NET_DEFAULT_BACKLOG));

    DEBUG_PRINTF("waiting to accept client\n");
    OK_OR_DIE(chif_net_accept(sock, &cliaddr, &clisock));

    char cliip[CHIF_NET_IPVX_STRING_LENGTH];
    chif_net_port cliport;
    OK_OR_DIE(
      chif_net_ip_from_address(&cliaddr, cliip, CHIF_NET_IPVX_STRING_LENGTH));
    OK_OR_DIE(chif_net_port_from_address(&cliaddr, &cliport));
    DEBUG_PRINTF("client connected from %s:%d\n", cliip, cliport);
  }

  DEBUG_PRINTF("waiting for messsage\n");
  enum
  {
    bufsize = 1024
  };
  uint8_t buf[bufsize];
  chif_net_ssize_t bytes;
  if (proto == CHIF_NET_TRANSPORT_PROTOCOL_TCP) {
    while (chif_net_read(clisock, buf, bufsize, &bytes) ==
           CHIF_NET_RESULT_SUCCESS && bytes > 0) {
      DEBUG_PRINTF("read [%s], echoing it back.\n", (char*)buf);
      chif_net_write(clisock, buf, (size_t)bytes, &bytes);
    }
  } else {
    OK_OR_DIE(chif_net_readfrom(sock, buf, bufsize, &bytes, &cliaddr));

    char srcip[CHIF_NET_IPVX_STRING_LENGTH];
    OK_OR_DIE(
      chif_net_ip_from_address(&cliaddr, srcip, CHIF_NET_IPVX_STRING_LENGTH));
    chif_net_port srcport;
    OK_OR_DIE(chif_net_port_from_address(&cliaddr, &srcport));
    DEBUG_PRINTF(
      "read [%s] from [%s:%d], echoing it back.\n", (char*)buf, srcip, srcport);

    OK_OR_DIE(chif_net_writeto(sock, buf, (size_t)bytes, &bytes, &cliaddr));
  }

  DEBUG_PRINTF("closing sockets\n");
  chif_net_close_socket(&clisock);
  chif_net_close_socket(&sock);

  return 0;
}

int
run_client(int argc, char** argv)
{
  // parse settings, if any
  chif_net_address_family af = CHIF_NET_ADDRESS_FAMILY_IPV4;
  chif_net_transport_protocol proto = CHIF_NET_TRANSPORT_PROTOCOL_TCP;
  chif_net_port port = 1337;
  char* ip = NULL;
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
        case 'h': { // HOSTNAME,  "127.0.0.1" or "duckduckgo.com"
          if (i + 1 < argc) {
            ip = argv[i + 1];
          }
          break;
        }
      }
    }
  }
  if (!ip) {
    ip = "localhost";
  }

  DEBUG_PRINTF("open socket with protocol [%s], address family [%s]\n",
               chif_net_transport_protocol_to_string(proto),
               chif_net_address_family_to_string(af));
  chif_net_socket sock;
  OK_OR_DIE(chif_net_open_socket(&sock, proto, af));

  DEBUG_PRINTF("create address [%s:%u]\n", ip, port);
  chif_net_address addr;
  enum
  {
    portstrlen = 6
  };
  char portstr[portstrlen];
  snprintf(portstr, portstrlen, "%u%c", port, '\0');
  OK_OR_DIE(chif_net_create_address(&addr, ip, portstr, af, proto));

  DEBUG_PRINTF("connecting..\n");
  OK_OR_DIE(chif_net_connect(sock, &addr));
  DEBUG_PRINTF(".. connected\n");

  const char* str = "chif_net is cool!";
  DEBUG_PRINTF("writing [%s]\n", str);
  enum
  {
    strlen = 18
  };
  chif_net_ssize_t written = 0;
  while (written < strlen) {
    chif_net_ssize_t bytes;
    // chif_net_write is not guaranteed to send all bytes in one call.
    OK_OR_DIE(chif_net_write(sock, (uint8_t*)str, strlen, &bytes));
    written += bytes;
  }

  enum
  {
    bufsize = 1024
  };
  uint8_t buf[bufsize];
  chif_net_ssize_t bytes;
  int can_read;
  chif_net_can_read(sock, &can_read, 100);
  OK_OR_DIE(can_read == CHIF_NET_TRUE);
  OK_OR_DIE(chif_net_read(sock, buf, bufsize, &bytes));
  DEBUG_PRINTF("read [%s]\n", (char*)buf);

  DEBUG_PRINTF("closing socket\n");
  chif_net_close_socket(&sock);

  return 0;
}

#undef OK_OR_DIE
