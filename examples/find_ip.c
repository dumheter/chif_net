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

void
find_server_bind_address()
{
  printf("open socket\n");
  chif_net_socket sock;
  const chif_net_address_family af = CHIF_NET_ADDRESS_FAMILY_IPV4;
  const chif_net_protocol proto = CHIF_NET_PROTOCOL_TCP;
  ok_or_die(chif_net_open_socket(&sock, proto, af));

  printf("bind socket\n");
  const chif_net_port port = CHIF_NET_UNUSED_PORT;
  ok_or_die(chif_net_bind(sock, port, af));

  printf("listen for connection\n");
  ok_or_die(chif_net_listen(sock, CHIF_NET_DEFAULT_BACKLOG));

  {
    printf("ip and port from socket\n");
    char ip[CHIF_NET_IPVX_STRING_LENGTH];
    ok_or_die(chif_net_ip_from_socket(sock, ip, CHIF_NET_IPVX_STRING_LENGTH));
    chif_net_port p;
    ok_or_die(chif_net_port_from_socket(sock, &p));
    printf("\t%s:%d\n", ip, p);
  }

  {
    printf("ip and port from address\n");
    chif_net_address addr;
    ok_or_die(chif_net_get_address(sock, &addr));
    char ip[CHIF_NET_IPVX_STRING_LENGTH];
    ok_or_die(chif_net_ip_from_address(&addr, ip, CHIF_NET_IPVX_STRING_LENGTH));
    chif_net_port p;
    ok_or_die(chif_net_port_from_address(&addr, &p));
    printf("\t%s:%d\n", ip, p);
  }

  printf("closing sockets\n");
  chif_net_close_socket(&sock);
}

void
find_LAN_address()
{
  printf("open socket\n");
  chif_net_socket sock;
  const chif_net_address_family af = CHIF_NET_ADDRESS_FAMILY_IPV4;
  const chif_net_protocol proto = CHIF_NET_PROTOCOL_TCP;
  ok_or_die(chif_net_open_socket(&sock, proto, af));

  printf("connecting to Google's DNS\n");
  chif_net_address google_dns_addr;
#define GOOGLE_DNS_IP "8.8.8.8"
#define DNS_PORT 53
  ok_or_die(
    chif_net_create_address(&google_dns_addr, GOOGLE_DNS_IP, DNS_PORT, af));
  ok_or_die(chif_net_connect(sock, &google_dns_addr));

  {
    printf("ip and port from socket\n");
    char ip[CHIF_NET_IPVX_STRING_LENGTH];
    ok_or_die(chif_net_ip_from_socket(sock, ip, CHIF_NET_IPVX_STRING_LENGTH));
    chif_net_port p;
    ok_or_die(chif_net_port_from_socket(sock, &p));
    printf("\t%s:%d\n", ip, p);
  }

  {
    printf("peer ip and port from address\n");
    chif_net_address addr;
    ok_or_die(chif_net_get_peer_address(sock, &addr));
    char ip[CHIF_NET_IPVX_STRING_LENGTH];
    ok_or_die(chif_net_ip_from_address(&addr, ip, CHIF_NET_IPVX_STRING_LENGTH));
    chif_net_port p;
    ok_or_die(chif_net_port_from_address(&addr, &p));
    printf("\t%s:%d\n", ip, p);
  }

  printf("closing sockets\n");
  chif_net_close_socket(&sock);
}

int
main(int argc, char** argv)
{
  chif_net_startup();

  printf("== Find what address the server binds to locally\n");
  find_server_bind_address();

  printf("\n== Find our LAN address\n");
  find_LAN_address();

  printf("exiting\n");
  chif_net_shutdown();

#if defined(_WIN32) || defined(_WIN64)
  printf("\nenter any key to exit\n> ");
  int in = getchar();
#endif

  return 0;
}
