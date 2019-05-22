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
#include <stdio.h>
#include <string.h>

typedef struct
{
  chif_net_address_family af;
  chif_net_transport_protocol proto;
  chif_net_port port;
  const char* addr;
} echo_args;

void
run_echo_test(AlfTestState* state, const echo_args* args)
{
  enum
  {
    portstrlen = 6
  };
  char portstr[portstrlen];
  snprintf(portstr, portstrlen, "%u%c", args->port, '\0');

  chif_net_socket server;
  OK_OR_RET(chif_net_open_socket(&server, args->proto, args->af));
  OK_OR_RET(chif_net_set_reuse_addr(server, CHIF_NET_TRUE));

  chif_net_address server_addr;
  OK_OR_RET(chif_net_create_address(
    &server_addr, CHIF_NET_ANY_ADDRESS, portstr, args->af, args->proto));

  OK_OR_RET(chif_net_bind(server, &server_addr));

  if (args->proto == CHIF_NET_TRANSPORT_PROTOCOL_TCP) {
    OK_OR_RET(chif_net_listen(server, CHIF_NET_DEFAULT_BACKLOG));
  }

  chif_net_socket client;
  OK_OR_RET(chif_net_open_socket(&client, args->proto, args->af));

  chif_net_address client_addr;
  OK_OR_RET(chif_net_create_address(
    &client_addr, args->addr, portstr, args->af, args->proto));

  OK_OR_RET(chif_net_connect(client, &client_addr));

  chif_net_port p;
  chif_net_port_from_address(&client_addr, &p);

  chif_net_socket server_client = CHIF_NET_INVALID_SOCKET;
  if (args->proto == CHIF_NET_TRANSPORT_PROTOCOL_TCP) {
    int can_read;
    OK_OR_RET(chif_net_can_read(server, &can_read, 50));
    OK_OR_RET(can_read == CHIF_NET_TRUE);

    chif_net_address unused;
    if (args->af == CHIF_NET_ADDRESS_FAMILY_IPV4) {
      unused.address_family = CHIF_NET_ADDRESS_FAMILY_IPV4;
    } else {
      unused.address_family = CHIF_NET_ADDRESS_FAMILY_IPV6;
    }
    OK_OR_RET(chif_net_accept(server, &unused, &server_client));
  }

  enum
  {
    buflen = 18
  };
  char buf[buflen] = "this is a message";
  int bytes;
  OK_OR_RET(chif_net_write(client, (uint8_t*)buf, buflen, &bytes));
  OK_OR_RET(buflen == bytes)

  enum
  {
    inbuflen = 20
  };
  char inbuf[inbuflen];
  int inbytes = 0;
  int can_read = 0;
  chif_net_socket* cli =
    args->proto == CHIF_NET_TRANSPORT_PROTOCOL_TCP ? &server_client : &server;
  OK_OR_RET(chif_net_can_read(*cli, &can_read, 50));
  OK_OR_RET(can_read == CHIF_NET_TRUE);
  chif_net_address from_addr;
  CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(from_addr);
  if (args->proto == CHIF_NET_TRANSPORT_PROTOCOL_TCP) {
    OK_OR_RET(chif_net_read(*cli, (uint8_t*)inbuf, inbuflen, &inbytes));
  } else {
    if (args->af == CHIF_NET_ADDRESS_FAMILY_IPV4) {
      from_addr.address_family = CHIF_NET_ADDRESS_FAMILY_IPV4;
    } else {
      from_addr.address_family = CHIF_NET_ADDRESS_FAMILY_IPV6;
    }
    OK_OR_RET(
      chif_net_readfrom(*cli, (uint8_t*)inbuf, inbuflen, &inbytes, &from_addr));
  }
  OK_OR_RET(inbytes == bytes);
  OK_OR_RET(memcmp(buf, inbuf, inbytes) == 0);

  int bytes2;
  if (args->proto == CHIF_NET_TRANSPORT_PROTOCOL_TCP) {
    OK_OR_RET(chif_net_write(*cli, (uint8_t*)inbuf, inbytes, &bytes2));
  } else {
    OK_OR_RET(
      chif_net_writeto(*cli, (uint8_t*)inbuf, inbytes, &bytes2, &from_addr));
  }
  OK_OR_RET(bytes2 == inbytes);

  int inbytes2;
  OK_OR_RET(chif_net_can_read(client, &can_read, 50));
  OK_OR_RET(can_read == CHIF_NET_TRUE);
  OK_OR_RET(chif_net_read(client, (uint8_t*)buf, buflen, &inbytes2));
  OK_OR_RET(inbytes2 == bytes2);
  OK_OR_RET(memcmp(buf, inbuf, inbytes2) == 0);

  OK_OR_RET(chif_net_close_socket(&server));
  OK_OR_RET(chif_net_close_socket(&client));
  OK_OR_RET(chif_net_close_socket(cli));
}

void
tcp_ipv4(AlfTestState* state)
{
  echo_args args;
  args.af = CHIF_NET_ADDRESS_FAMILY_IPV4;
  args.proto = CHIF_NET_TRANSPORT_PROTOCOL_TCP;
  args.port = 1337;
  args.addr = "localhost";
  run_echo_test(state, &args);
}

void
tcp_ipv6(AlfTestState* state)
{
  echo_args args;
  args.af = CHIF_NET_ADDRESS_FAMILY_IPV6;
  args.proto = CHIF_NET_TRANSPORT_PROTOCOL_TCP;
  args.port = 1338;
  args.addr = "localhost";
  run_echo_test(state, &args);
}

void
udp_ipv4(AlfTestState* state)
{
  echo_args args;
  args.af = CHIF_NET_ADDRESS_FAMILY_IPV4;
  args.proto = CHIF_NET_TRANSPORT_PROTOCOL_UDP;
  args.port = 1339;
  args.addr = "localhost";
  run_echo_test(state, &args);
}

void
udp_ipv6(AlfTestState* state)
{
  echo_args args;
  args.af = CHIF_NET_ADDRESS_FAMILY_IPV6;
  args.proto = CHIF_NET_TRANSPORT_PROTOCOL_UDP;
  args.port = 1340;
  args.addr = "localhost";
  run_echo_test(state, &args);
}
