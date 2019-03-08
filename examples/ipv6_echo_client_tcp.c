#include "chif_net.h"
#include <stdio.h>
#include <stdlib.h>

void
ok_or_die(chif_net_result res)
{
  if (res != CHIF_NET_RESULT_SUCCESS) {
    printf("failed with error %s.\n", chif_net_result_to_string(res));
#if defined(_WIN32) || defined(_WIN64)
    printf("\nenter any key to exit\n> ");
    int in = getchar();
#endif
    exit(1);
  }
}

int
main(int argc, char** argv)
{
  chif_net_startup();

  printf("open socket\n");
  chif_net_socket sock;
  const chif_net_address_family af = CHIF_NET_ADDRESS_FAMILY_IPV6;
  const chif_net_protocol proto = CHIF_NET_PROTOCOL_TCP;
  ok_or_die(chif_net_open_socket(&sock, proto, af));

  printf("create address\n");
  const char* ip = "localhost";
  const char* port = "1337";
  chif_net_address addr;
  ok_or_die(chif_net_lookup_address(&addr, ip, port, af, proto));

  printf("connect\n");
  ok_or_die(chif_net_connect(sock, &addr));

  const char* str = "chif_net is cool!";
  printf("writing [%s]\n", str);
  enum
  {
    strlen = 18
  };
  ssize_t written = 0;
  while (written < strlen) {
    ssize_t bytes;
    // chif_net_write is not guaranteed to send all bytes in one call.
    ok_or_die(chif_net_write(sock, (uint8_t*)str, strlen, &bytes));
    written += bytes;
  }

  enum
  {
    bufsize = 1024
  };
  uint8_t buf[bufsize];
  ssize_t bytes;
  ok_or_die(chif_net_read(sock, buf, bufsize, &bytes));
  printf("read [%s]\n", (char*)buf);

  printf("closing socket\n");
  chif_net_close_socket(&sock);

  printf("exit\n");
  chif_net_shutdown();

#if defined(_WIN32) || defined(_WIN64)
  printf("\nenter any key to exit\n> ");
  int in = getchar();
#endif

  return 0;
}
