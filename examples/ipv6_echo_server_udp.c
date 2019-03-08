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
  const chif_net_protocol proto = CHIF_NET_PROTOCOL_UDP;
  ok_or_die(chif_net_open_socket(&sock, proto, af));

  printf("bind socket\n");
  const chif_net_port port = 1337;
  ok_or_die(chif_net_bind(sock, port, af));

  printf("waiting for message\n");
  enum
  {
    bufsize = 1024
  };
  uint8_t buf[bufsize];
  ssize_t bytes;
  chif_net_address srcaddr;
  ok_or_die(chif_net_readfrom(sock, buf, bufsize, &bytes, &srcaddr));
  char srcip[CHIF_NET_IPVX_STRING_LENGTH];
  ok_or_die(
    chif_net_ip_from_address(&srcaddr, srcip, CHIF_NET_IPVX_STRING_LENGTH));
  chif_net_port srcport;
  ok_or_die(chif_net_port_from_address(&srcaddr, &srcport));
  printf(
    "read [%s] from %s:%d, echoing it back.\n", (char*)buf, srcip, srcport);
  chif_net_writeto(sock, buf, (size_t)bytes, &bytes, &srcaddr);

  printf("closing socket\n");
  chif_net_close_socket(&sock);

  printf("exiting\n");
  chif_net_shutdown();

#if defined(_WIN32) || defined(_WIN64)
  printf("\nenter any key to exit\n> ");
  int in = getchar();
#endif

  return 0;
}
