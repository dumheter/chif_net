#include "echo.h"
#include <chif_net.h>
#include <stdio.h>

int
main(int argc, char** argv)
{
  chif_net_startup();
  printf("running echo server\n");

  run_server(argc, argv);

  printf("exiting\n");
  chif_net_shutdown();

#if defined(_WIN32) || defined(_WIN64)
  printf("\nenter any key to exit\n> ");
  const int in = getchar();
  CHIF_NET_SUPPRESS_UNUSED_VAR_WARNING(in);
#endif

  return 0;
}
