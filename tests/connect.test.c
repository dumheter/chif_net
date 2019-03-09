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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
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
#include "chif_net.h"
#include <stdlib.h>

void duckduckgo(AlfTestState* state)
{
  chif_net_socket sock;
  const chif_net_address_family af = CHIF_NET_ADDRESS_FAMILY_IPV4;
  const chif_net_protocol proto = CHIF_NET_PROTOCOL_TCP;
  ALF_CHECK_TRUE(state, CHIF_NET_RESULT_SUCCESS ==
                 chif_net_open_socket(&sock, proto, af));

  const char* site = "www.duckduckgo.com";
  chif_net_address addr;
  ALF_CHECK_TRUE(state, CHIF_NET_RESULT_SUCCESS ==
                 chif_net_lookup_address(&addr, site, "http", af, proto));
  ALF_CHECK_TRUE(state, CHIF_NET_RESULT_SUCCESS ==
                 chif_net_connect(sock, &addr));

  // some invalid request to get a 400 response.
  const char* request = "GET /robot.txt HTTP/1.1\
  Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*\\/\\*;q=0.8 \
  Accept-Language: en-US,en;q=0.5                                       \
  Accept-Encoding: gzip, deflate                                        \
  ";
  const size_t request_len = strlen(request);
  ssize_t written = 0;
  int iters = 0;
  while (written < request_len) {
    ssize_t bytes;
    ALF_CHECK_TRUE(state, CHIF_NET_RESULT_SUCCESS ==
                   chif_net_write(sock, (uint8_t*)request,
                                  request_len, &bytes));
    written += bytes;
    ++iters;
    ALF_CHECK_TRUE_R(state, iters < 5,
                     "looping too much, failing to send regular sized packets");
  }

  enum { buflen = 50 * (long)1e6}; // 50 MB
  uint8_t* buf = malloc(buflen);
  ssize_t readbytes;
  ALF_CHECK_TRUE(state, CHIF_NET_RESULT_SUCCESS ==
                 chif_net_read(sock, buf, buflen, &readbytes));
  ALF_CHECK_TRUE(state, readbytes > 100); // probably larger than 100 bytes
  free(buf);

  chif_net_close_socket(&sock);
}

void bad_site(AlfTestState* state)
{
  chif_net_socket sock;
  const chif_net_address_family af = CHIF_NET_ADDRESS_FAMILY_IPV4;
  const chif_net_protocol proto = CHIF_NET_PROTOCOL_TCP;
  ALF_CHECK_TRUE(state, CHIF_NET_RESULT_SUCCESS ==
                 chif_net_open_socket(&sock, proto, af));

  const char* site = "no site";
  chif_net_address addr;
  ALF_CHECK_FALSE_R(state, CHIF_NET_RESULT_SUCCESS ==
                  chif_net_lookup_address(&addr, site, "http", af, proto),
                  "attempting to lookup address no site");
  ALF_CHECK_FALSE(state, CHIF_NET_RESULT_SUCCESS ==
                  chif_net_connect(sock, &addr));

  chif_net_close_socket(&sock);
}
