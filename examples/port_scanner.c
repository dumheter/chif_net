#include "chif_net.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/timeb.h>

void
ok_or_die(chif_net_result res)
{
  if (res != CHIF_NET_RESULT_SUCCESS) {
    printf("failed with error %s.\n", chif_net_result_to_string(res));
    exit(1);
  }
}

void
print_help_and_die(char** argv)
{
  printf("Usage: %s [ip address]\n\nexample: %s 127.0.0.1\n", argv[0], argv[0]);
  exit(0);
}

int
main(int argc, char** argv)
{
  if (argc != 2)
    print_help_and_die(argv);

  chif_net_startup();

  printf("open socket\n");
  chif_net_socket sock;
  chif_net_protocol proto = CHIF_NET_PROTOCOL_TCP;
  chif_net_address_family af = CHIF_NET_ADDRESS_FAMILY_IPV4;

  printf("scanning for open tcp ports, with timeout of 1 ms.\n");
  chif_net_port port = 1;
  while (port) { // will terminate on overflow to 0
    ok_or_die(chif_net_open_socket(&sock, proto, af));

    chif_net_address addr;
    chif_net_result res = chif_net_create_address(&addr, argv[1], port, af);
    if (res != CHIF_NET_RESULT_SUCCESS) {
      printf("failed to create address, error %s.\n",
             chif_net_result_to_string(res));
      print_help_and_die(argv);
    }

    // if a connecting cannot be found, it will only timeout after tcp fails
    // to connect, which can take several seconds. Therefore we dissable
    // blocking and use the timeout in chif_net_can_write function to timeout.
    ok_or_die(chif_net_set_socket_blocking(sock, false));
    res = chif_net_connect(sock, &addr);
    if (res == CHIF_NET_RESULT_WOULD_BLOCK || res == CHIF_NET_RESULT_SUCCESS) {
      int is_connected;
      res = chif_net_can_write(sock, &is_connected, 1);
      if (is_connected) {
        printf("port %d open\n", port);
      }
    } else {
      ok_or_die(res);
    }

    chif_net_close_socket(&sock);
    port++;
  }

  printf("exiting\n");
  chif_net_shutdown();
  return 0;
}
