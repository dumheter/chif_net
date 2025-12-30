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

void
poll_test(AlfTestState* state)
{
  const int timeout_ms = 50;
  int ready_count;
  chif_net_check check;

  { // polling an invalid socket
    check.socket = CHIF_NET_INVALID_SOCKET;
    check.request_events =
      CHIF_NET_CHECK_EVENT_READ | CHIF_NET_CHECK_EVENT_WRITE;
    check.return_events = 0;

    const chif_net_result res =
      chif_net_poll(&check, 1, &ready_count, timeout_ms);
    ALF_CHECK_TRUE(state, ready_count == 0 || res != CHIF_NET_RESULT_SUCCESS);
  }

  { // polling a opened, then closed socket
    chif_net_socket socket;
    OK_OR_RET(chif_net_open_socket(
      &socket, CHIF_NET_TRANSPORT_PROTOCOL_TCP, CHIF_NET_ADDRESS_FAMILY_IPV4));
    OK_OR_RET(chif_net_set_reuse_addr(socket, CHIF_NET_TRUE));
    OK_OR_RET(chif_net_close_socket(&socket));
    check.socket = socket;
    check.request_events =
      CHIF_NET_CHECK_EVENT_READ | CHIF_NET_CHECK_EVENT_WRITE;
    check.return_events = 0;

    const chif_net_result res =
      chif_net_poll(&check, 1, &ready_count, timeout_ms);
    ALF_CHECK_TRUE(state, ready_count == 0 || res != CHIF_NET_RESULT_SUCCESS);
  }

  // ============================================================ //

  { // polling an open ipv6 udp socket
    chif_net_socket socket;
    OK_OR_RET(chif_net_open_socket(
      &socket, CHIF_NET_TRANSPORT_PROTOCOL_UDP, CHIF_NET_ADDRESS_FAMILY_IPV6));
    OK_OR_RET(chif_net_set_reuse_addr(socket, CHIF_NET_TRUE));
    check.socket = socket;
    check.request_events =
      CHIF_NET_CHECK_EVENT_READ | CHIF_NET_CHECK_EVENT_WRITE;
    check.return_events = 0;
    OK_OR_RET(chif_net_poll(&check, 1, &ready_count, timeout_ms));

    const int read = check.return_events & CHIF_NET_CHECK_EVENT_READ;
    const int write = check.return_events & CHIF_NET_CHECK_EVENT_WRITE;
    const int error = check.return_events & CHIF_NET_CHECK_EVENT_ERROR;
    const int closed = check.return_events & CHIF_NET_CHECK_EVENT_CLOSED;
    const int invalid = check.return_events & CHIF_NET_CHECK_EVENT_INVALID;
    ALF_CHECK_TRUE(state, ready_count == 1);
    ALF_CHECK_TRUE(state, read == 0);
    ALF_CHECK_TRUE(state, write > 0);
    ALF_CHECK_TRUE(state, error == 0);
    ALF_CHECK_TRUE(state, closed == 0);
    ALF_CHECK_TRUE(state, invalid == 0);

    OK_OR_RET(chif_net_close_socket(&socket));
  }

  { // polling an open ipv4 udp socket
    chif_net_socket socket;
    OK_OR_RET(chif_net_open_socket(
      &socket, CHIF_NET_TRANSPORT_PROTOCOL_UDP, CHIF_NET_ADDRESS_FAMILY_IPV4));
    OK_OR_RET(chif_net_set_reuse_addr(socket, CHIF_NET_TRUE));
    check.socket = socket;
    check.request_events =
      CHIF_NET_CHECK_EVENT_READ | CHIF_NET_CHECK_EVENT_WRITE;
    check.return_events = 0;
    OK_OR_RET(chif_net_poll(&check, 1, &ready_count, timeout_ms));

    const int read = check.return_events & CHIF_NET_CHECK_EVENT_READ;
    const int write = check.return_events & CHIF_NET_CHECK_EVENT_WRITE;
    const int error = check.return_events & CHIF_NET_CHECK_EVENT_ERROR;
    const int closed = check.return_events & CHIF_NET_CHECK_EVENT_CLOSED;
    const int invalid = check.return_events & CHIF_NET_CHECK_EVENT_INVALID;
    ALF_CHECK_TRUE(state, ready_count == 1);
    ALF_CHECK_TRUE(state, read == 0);
    ALF_CHECK_TRUE(state, write > 0);
    ALF_CHECK_TRUE(state, error == 0);
    ALF_CHECK_TRUE(state, closed == 0);
    ALF_CHECK_TRUE(state, invalid == 0);

    OK_OR_RET(chif_net_close_socket(&socket));
  }

  { // polling one open and two closed ipv4 udp socket
    chif_net_socket socket;
    OK_OR_RET(chif_net_open_socket(
      &socket, CHIF_NET_TRANSPORT_PROTOCOL_UDP, CHIF_NET_ADDRESS_FAMILY_IPV4));
    OK_OR_RET(chif_net_set_reuse_addr(socket, CHIF_NET_TRUE));
    chif_net_check checks[3];
    checks[0].socket = CHIF_NET_INVALID_SOCKET;
    checks[0].request_events =
      CHIF_NET_CHECK_EVENT_READ | CHIF_NET_CHECK_EVENT_WRITE;
    checks[0].return_events = 0;
    checks[1].socket = socket;
    checks[1].request_events =
      CHIF_NET_CHECK_EVENT_READ | CHIF_NET_CHECK_EVENT_WRITE;
    checks[1].return_events = 0;
    checks[2].socket = CHIF_NET_INVALID_SOCKET;
    checks[2].request_events =
      CHIF_NET_CHECK_EVENT_READ | CHIF_NET_CHECK_EVENT_WRITE;
    checks[2].return_events = 0;
    OK_OR_RET(chif_net_poll(checks, 3, &ready_count, timeout_ms));
    ALF_CHECK_TRUE(state, ready_count == 1);
    ALF_CHECK_FALSE(state, checks[1].return_events & CHIF_NET_CHECK_EVENT_READ);
    ALF_CHECK_TRUE(state, checks[1].return_events & CHIF_NET_CHECK_EVENT_WRITE);
    ALF_CHECK_FALSE(state,
                    checks[1].return_events & CHIF_NET_CHECK_EVENT_ERROR);
    ALF_CHECK_FALSE(state,
                    checks[1].return_events & CHIF_NET_CHECK_EVENT_CLOSED);
    ALF_CHECK_FALSE(state,
                    checks[1].return_events & CHIF_NET_CHECK_EVENT_INVALID);

    OK_OR_RET(chif_net_close_socket(&socket));
  }

  // ============================================================ //

  { // Two sockets send data to each other, and poll shows can READ, IPv4 UDP
    chif_net_socket socka;
    OK_OR_RET(chif_net_open_socket(
      &socka, CHIF_NET_TRANSPORT_PROTOCOL_UDP, CHIF_NET_ADDRESS_FAMILY_IPV4));
    OK_OR_RET(chif_net_set_reuse_addr(socka, CHIF_NET_TRUE));
    chif_net_socket sockb;
    OK_OR_RET(chif_net_open_socket(
      &sockb, CHIF_NET_TRANSPORT_PROTOCOL_UDP, CHIF_NET_ADDRESS_FAMILY_IPV4));
    OK_OR_RET(chif_net_set_reuse_addr(sockb, CHIF_NET_TRUE));

    chif_net_address anyaddr;
    anyaddr.address_family = CHIF_NET_ADDRESS_FAMILY_IPV4;
    OK_OR_RET(chif_net_create_address(&anyaddr,
                                      "localhost",
                                      CHIF_NET_ANY_PORT,
                                      CHIF_NET_TRANSPORT_PROTOCOL_UDP,
                                      CHIF_NET_ADDRESS_FAMILY_IPV4));

    OK_OR_RET(chif_net_bind(socka, &anyaddr));
    OK_OR_RET(chif_net_bind(sockb, &anyaddr));

    chif_net_address addra;
    addra.address_family = CHIF_NET_ADDRESS_FAMILY_IPV4;
    OK_OR_RET(chif_net_address_from_socket(socka, &addra));
    chif_net_address addrb;
    addrb.address_family = CHIF_NET_ADDRESS_FAMILY_IPV4;
    OK_OR_RET(chif_net_address_from_socket(sockb, &addrb));

    enum
    {
      bufsize = 6
    };
    uint8_t buf[bufsize] = { 0, 1, 2, 3, 4, 5 };
    int bytes;

    OK_OR_RET(chif_net_writeto(socka, buf, bufsize, &bytes, &addrb));
    OK_OR_RET(chif_net_writeto(sockb, buf, bufsize, &bytes, &addra));

    chif_net_check checks[2];
    checks[0].socket = socka;
    checks[0].request_events =
      CHIF_NET_CHECK_EVENT_READ | CHIF_NET_CHECK_EVENT_WRITE;
    checks[0].return_events = 0;
    checks[1].socket = sockb;
    checks[1].request_events =
      CHIF_NET_CHECK_EVENT_READ | CHIF_NET_CHECK_EVENT_WRITE;
    checks[1].return_events = 0;
    OK_OR_RET(chif_net_poll(checks, 2, &ready_count, timeout_ms));

    ALF_CHECK_TRUE(state, ready_count == 2);
    for (int i = 0; i < 2; i++) {
      ALF_CHECK_TRUE(state,
                     checks[i].return_events & CHIF_NET_CHECK_EVENT_READ);
      ALF_CHECK_TRUE(state,
                     checks[i].return_events & CHIF_NET_CHECK_EVENT_WRITE);
      ALF_CHECK_FALSE(state,
                      checks[i].return_events & CHIF_NET_CHECK_EVENT_ERROR);
      ALF_CHECK_FALSE(state,
                      checks[i].return_events & CHIF_NET_CHECK_EVENT_CLOSED);
      ALF_CHECK_FALSE(state,
                      checks[i].return_events & CHIF_NET_CHECK_EVENT_INVALID);
    }

    OK_OR_RET(chif_net_close_socket(&socka));
    OK_OR_RET(chif_net_close_socket(&sockb));
  }

  // ============================================================ //

  /* enum */
  /* { */
  /*   portstrlen = 6 */
  /* }; */
  /* char portstr[portstrlen]; */
  /* snprintf(portstr, portstrlen, "%d%c", args->port, '\0'); */
}
