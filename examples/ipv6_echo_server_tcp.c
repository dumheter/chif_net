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

  printf("bind socket\n");
  const chif_net_port port = 1337;
  ok_or_die(chif_net_bind(sock, port, af));

  chif_net_port bound_port;
  ok_or_die(chif_net_port_from_socket(sock, &bound_port));
  char bound_ip[CHIF_NET_IPVX_STRING_LENGTH];
  ok_or_die(
    chif_net_ip_from_socket(sock, bound_ip, CHIF_NET_IPVX_STRING_LENGTH));
  printf("socket bound on %s:%u\n", bound_ip, bound_port);

  printf("listen for connection\n");
  ok_or_die(chif_net_listen(sock, CHIF_NET_DEFAULT_BACKLOG));

  printf("waiting to accept client\n");
  chif_net_socket clisock;
  chif_net_address cliaddr;
  ok_or_die(chif_net_accept(sock, &cliaddr, &clisock));

  char cliip[CHIF_NET_IPVX_STRING_LENGTH];
  chif_net_port cliport;
  ok_or_die(
    chif_net_ip_from_address(&cliaddr, cliip, CHIF_NET_IPVX_STRING_LENGTH));
  ok_or_die(chif_net_port_from_address(&cliaddr, &cliport));
  printf("client connected from %s:%d\n\n", cliip, cliport);

  enum
  {
    bufsize = 1024
  };
  uint8_t buf[bufsize];
  ssize_t bytes;
  while (chif_net_read(clisock, buf, bufsize, &bytes) ==
         CHIF_NET_RESULT_SUCCESS) {
    printf("read [%s], echoing it back.\n", (char*)buf);
    chif_net_write(clisock, buf, (size_t)bytes, &bytes);
  }

  printf("closing sockets\n");
  chif_net_close_socket(&clisock);
  chif_net_close_socket(&sock);

  printf("exiting\n");
  chif_net_shutdown();

#if defined(_WIN32) || defined(_WIN64)
  printf("\nenter any key to exit\n> ");
  int in = getchar();
#endif

  return 0;
}
