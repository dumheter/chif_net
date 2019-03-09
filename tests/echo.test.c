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
#include "../examples/echo.h"
#include "chif_net.h"
#include "alf_thread.h"
#include <stdlib.h>


struct Args
{
  int argc;
  char** argv;
};

uint32_t server_helper(void* args)
{
  uint32_t res = run_server(((struct Args*)args)->argc, ((struct Args*)args)->argv);
  return res;
}

void tcp_ipv4(AlfTestState* state)
{
  char* server_argv[] = {"IGNORED", "-t", "-4", "-p", "1337"};
  struct Args args = (struct Args){5, server_argv};
  AlfThread* server_thread =
      alfCreateThread(server_helper, (void*)&args);

  char* argv[] = {"IGNORED", "-t", "-4", "-p", "1337", "-i", "localhost"};
  const int argc = 7;
  int client_res = run_client(argc, argv);
  ALF_CHECK_TRUE(state, client_res == 0);

  uint32_t server_res = -1;
  if (client_res == 0) {
    server_res = alfJoinThread(server_thread);
  }
  else {
    alfKillThread(server_thread);
  }
  ALF_CHECK_TRUE(state, server_res == 0);
}

void tcp_ipv6(AlfTestState* state)
{
  char* server_argv[] = {"IGNORED", "-t", "-6", "-p", "1337"};
  struct Args args = (struct Args){5, server_argv};
  AlfThread* server_thread =
      alfCreateThread(server_helper, (void*)&args);

  char* argv[] = {"IGNORED", "-t", "-6", "-p", "1337", "-i", "localhost"};
  const int argc = 7;
  int client_res = run_client(argc, argv);
  ALF_CHECK_TRUE(state, client_res == 0);

  uint32_t server_res = -1;
  if (client_res == 0) {
    server_res = alfJoinThread(server_thread);
  }
  else {
    alfKillThread(server_thread);
  }
  ALF_CHECK_TRUE(state, server_res == 0);
}

void udp_ipv4(AlfTestState* state)
{
  char* server_argv[] = {"IGNORED", "-u", "-4", "-p", "1337"};
  struct Args args = (struct Args){5, server_argv};
  AlfThread* server_thread =
      alfCreateThread(server_helper, (void*)&args);

  char* argv[] = {"IGNORED", "-u", "-4", "-p", "1337", "-i", "localhost"};
  const int argc = 7;
  int client_res = run_client(argc, argv);
  ALF_CHECK_TRUE(state, client_res == 0);

  uint32_t server_res = -1;
  if (client_res == 0) {
    server_res = alfJoinThread(server_thread);
  }
  else {
    alfKillThread(server_thread);
  }
  ALF_CHECK_TRUE(state, server_res == 0);
}

void udp_ipv6(AlfTestState* state)
{
  char* server_argv[] = {"IGNORED", "-u", "-6", "-p", "1337"};
  struct Args args = (struct Args){5, server_argv};
  AlfThread* server_thread =
      alfCreateThread(server_helper, (void*)&args);

  char* argv[] = {"IGNORED", "-u", "-6", "-p", "1337", "-i", "localhost"};
  const int argc = 7;
  int client_res = run_client(argc, argv);
  ALF_CHECK_TRUE(state, client_res == 0);

  uint32_t server_res = -1;
  if (client_res == 0) {
    server_res = alfJoinThread(server_thread);
  }
  else {
    alfKillThread(server_thread);
  }
  ALF_CHECK_TRUE(state, server_res == 0);
}
