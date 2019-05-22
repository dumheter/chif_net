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

#define ALF_TEST_THEME_AUTUMN_FRUIT
#include "tests.h"
#include <alf_test.h>
#include <alf_thread.h>
#include <chif_net.h>

int
main()
{
  chif_net_startup();
  alfThreadStartup();

  enum
  {
    suites_count = 4
  };
  AlfTestSuite* suites[suites_count];

  // ============================================================ //
  // connect
  // ============================================================ //
  enum
  {
    connect_tests_count = 2
  };
  AlfTest connect_tests[connect_tests_count];
  connect_tests[0] =
    (AlfTest){ .name = "duckduckgo", .TestFunction = duckduckgo };
  connect_tests[1] = (AlfTest){ .name = "bad_site", .TestFunction = bad_site };
  suites[0] = alfCreateTestSuite("Connect", connect_tests, connect_tests_count);

  // ============================================================ //
  // tcp
  // ============================================================ //
  enum
  {
    tcp_tests_count = 1
  };
  AlfTest tcp_tests[tcp_tests_count];
  tcp_tests[0] =
    (AlfTest){ .name = "tcp", .TestFunction = tcp_test };
  suites[1] = alfCreateTestSuite("tcp", tcp_tests, tcp_tests_count);

  // ============================================================ //
  // tcp
  // ============================================================ //
  enum
  {
    poll_tests_count = 1
  };
  AlfTest poll_tests[poll_tests_count];
  poll_tests[0] = (AlfTest){ .name = "poll", .TestFunction = poll_test };
  suites[2] = alfCreateTestSuite("poll", poll_tests, poll_tests_count);

  // ============================================================ //
  // echo
  // ============================================================ //
  enum
  {
    echo_tests_count = 4
  };
  AlfTest echo_tests[echo_tests_count];
  echo_tests[0] = (AlfTest){ .name = "tcp & ipv4", .TestFunction = tcp_ipv4 };
  echo_tests[2] = (AlfTest){ .name = "tcp & ipv6", .TestFunction = tcp_ipv6 };
  echo_tests[1] = (AlfTest){ .name = "udp & ipv4", .TestFunction = udp_ipv4 };
  echo_tests[3] = (AlfTest){ .name = "udp & ipv6", .TestFunction = udp_ipv6 };
  suites[3] = alfCreateTestSuite("Echo", echo_tests, echo_tests_count);

  const uint32_t fails = alfRunSuites(suites, suites_count);
  for (int i = 0; i < suites_count; i++) {
    alfDestroyTestSuite(suites[i]);
  }

  alfThreadShutdown();
  chif_net_shutdown();
  return fails;
}
