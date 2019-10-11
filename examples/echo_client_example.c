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
#include <stdio.h>
#include <stdlib.h>

// ============================================================ //

int
run_client(int argc, char** argv);

// ============================================================ //

int
main(int argc, char** argv)
{
  chif_net_startup();
  printf("running echo client\n");

  const int ret = run_client(argc, argv);

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

  printf("open socket with protocol [%s], address family [%s]\n",
               chif_net_transport_protocol_to_string(proto),
               chif_net_address_family_to_string(af));
  chif_net_socket sock;
  OK_OR_CRASH(chif_net_open_socket(&sock, proto, af));

  printf("create address [%s:%u]\n", ip, port);
  chif_net_address addr;
  OK_OR_CRASH(chif_net_create_address_i(&addr, ip, port, af, proto));

  printf("connecting..\n");
  OK_OR_CRASH(chif_net_connect(sock, &addr));
  printf(".. connected\n");

  const char* str = "chif_net is cool!";
  printf("writing [%s]\n", str);
  enum
  {
    strlen = 18
  };
  int written = 0;
  while (written < strlen) {
    int bytes;
    // chif_net_write is not guaranteed to send all bytes in one call.
    OK_OR_CRASH(chif_net_write(sock, (uint8_t*)str, strlen, &bytes));
    written += bytes;
  }

  enum
  {
    bufsize = 1024
  };
  uint8_t buf[bufsize];
  int bytes;
  int can_read;
  chif_net_can_read(sock, &can_read, 100);
  OK_OR_CRASH(can_read == CHIF_NET_TRUE);
  OK_OR_CRASH(chif_net_read(sock, buf, bufsize, &bytes));
  printf("read [%s]\n", (char*)buf);

  printf("closing socket\n");
  chif_net_close_socket(&sock);

  return 0;
}
