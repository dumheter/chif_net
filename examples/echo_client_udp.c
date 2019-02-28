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
  const chif_net_address_family af = CHIF_NET_ADDRESS_FAMILY_IPV4;
  const chif_net_protocol proto = CHIF_NET_PROTOCOL_UDP;
  ok_or_die(chif_net_open_socket(&sock, proto, af));

  printf("create address\n");
  const char* ip = "127.0.0.1";
  const chif_net_port port = 1337;
  chif_net_address addr;
  ok_or_die(chif_net_create_address(&addr, ip, port, af));

  const char* str = "chif_net is cool!";
  printf("writing [%s]\n", str);
  enum
  {
    strlen = 18
  };
  ssize_t bytes;
  ok_or_die(chif_net_writeto(sock, (uint8_t*)str, strlen, &bytes, &addr));

  enum
  {
    bufsize = 1024
  };
  uint8_t buf[bufsize];
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
