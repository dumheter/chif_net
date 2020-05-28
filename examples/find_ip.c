#include <chif_net.h>
#include <stdio.h>
#include <stdlib.h>

void
ok_or_die(chif_net_result res)
{
  if (res != CHIF_NET_RESULT_SUCCESS) {
    printf("failed with error %s.\n", chif_net_result_to_string(res));
#if defined(_WIN32) || defined(_WIN64)
    printf("\nenter any key to exit\n> ");
    const int in = getchar();
    CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(in);
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
  const chif_net_transport_protocol proto = CHIF_NET_TRANSPORT_PROTOCOL_TCP;
  ok_or_die(chif_net_open_socket(&sock, proto, af));

  printf("bind socket\n");
  chif_net_address bind_addr;
  ok_or_die(chif_net_create_address(
      &bind_addr, "localhost", CHIF_NET_ANY_PORT, proto, af));
  ok_or_die(chif_net_bind(sock, &bind_addr));

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
    ok_or_die(chif_net_address_from_socket(sock, &addr));
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
  const chif_net_transport_protocol proto = CHIF_NET_TRANSPORT_PROTOCOL_TCP;
  ok_or_die(chif_net_open_socket(&sock, proto, af));

  printf("connecting to Google's DNS\n");
  chif_net_address google_dns_addr;
#define GOOGLE_DNS_IP "8.8.8.8"

  const char* dns_port = "53";

  ok_or_die(chif_net_create_address(
    &google_dns_addr, GOOGLE_DNS_IP, dns_port, proto, af));
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
    ok_or_die(chif_net_peer_address_from_socket(sock, &addr));
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
find_hostname_address(const char* site)
{
  printf("open socket\n");
  chif_net_socket sock;
  const chif_net_address_family af = CHIF_NET_ADDRESS_FAMILY_IPV4;
  const chif_net_transport_protocol proto = CHIF_NET_TRANSPORT_PROTOCOL_TCP;
  ok_or_die(chif_net_open_socket(&sock, proto, af));

  {
    printf("looking up %s 's ip\n", site);
    chif_net_address addr;
    ok_or_die(chif_net_create_address(&addr, site, "http", proto, af));
    ok_or_die(chif_net_connect(sock, &addr));
  }

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
    ok_or_die(chif_net_address_from_socket(sock, &addr));
    char ip[CHIF_NET_IPVX_STRING_LENGTH];
    ok_or_die(chif_net_ip_from_address(&addr, ip, CHIF_NET_IPVX_STRING_LENGTH));
    chif_net_port p;
    ok_or_die(chif_net_port_from_address(&addr, &p));
    printf("\t%s:%d\n", ip, p);
  }

  {
    printf("peer ip and port from address\n");
    chif_net_address addr;
    ok_or_die(chif_net_peer_address_from_socket(sock, &addr));
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
  CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(argc);
  CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(argv);
  chif_net_startup();

  printf("== Find what address the server binds to locally\n");
  find_server_bind_address();

  printf("\n== Find our LAN address\n");
  find_LAN_address();

  const char* site = "www.duckduckgo.com";
  printf("\n== Find %s 's IP address\n", site);
  find_hostname_address(site);

  printf("exiting\n");
  chif_net_shutdown();

#if defined(_WIN32) || defined(_WIN64)
  printf("\nenter any key to exit\n> ");
  const int in = getchar();
  CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(in);
#endif

  return 0;
}
