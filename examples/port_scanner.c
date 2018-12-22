#include "chif_net.h"
#include <stdlib.h>
#include <stdio.h>

void ok_or_die(chif_net_result res)
{
  if (res != CHIF_NET_RESULT_SUCCESS) {
    printf("failed with error %s.\n", chif_net_result_to_string(res));
    exit(1);
  }
}

void print_help_and_die(char** argv)
{
  printf("Usage: %s [ip address]\n\nexample: %s 127.0.0.1\n",
         argv[0], argv[0]);
  exit(0);
}

int main(int argc, char** argv)
{
  if (argc != 2) print_help_and_die(argv);

  chif_net_startup();

  printf("open socket\n");
  chif_net_socket sock;
  chif_net_protocol proto = CHIF_NET_PROTOCOL_TCP;
  chif_net_address_family af = CHIF_NET_ADDRESS_FAMILY_IPV4; 
  
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
    
    res = chif_net_connect(sock, &addr);
    if (res == CHIF_NET_RESULT_SUCCESS) {
      printf("port %d open\n", port);
    }

    chif_net_close_socket(&sock);
    port++;
  } 

  printf("exiting\n");
  chif_net_shutdown(); 
  return 0;
}
