#include "w_node.h"

int main(int argc, char **argv)
{
  int opt;
  int *flags = (int *)calloc(3, sizeof(int));

  while ( (opt = getopt(argc, argv, "n:p:")) != -1)
  {
    switch(opt)
    {
      case 'n':
        strncpy(node_info.address, optarg, LEN_ADDRESS);
        flags[0] = 1;
        break;

      case 'p':
        node_info.port = (unsigned short)atoi(optarg);
        flags[1] = 1;
        break;

      default:
        flags[2] = 1;
        usage(MESSAGE);
        break;
    }
  }
  if( !flags[1] || flags[2] )
    usage(MESSAGE);

  if(!flags[0])
    strncpy(node_info.address, "127.0.0.1", LEN_ADDRESS);

  free(flags);

  wallet_amount = 0.0;

  fillAddressIPv4(&node_address, node_info.address, node_info.port);
  printf("\nAuthenticated, receiving peer 2 connect...\n\n");

  //TODO: make connection to NODE_N (proc wallet);
  node_info.fd = Socket(AF_INET, SOCK_STREAM, 0);
  Connect(node_info.fd, (struct sockaddr *)&node_address);
  //sendInt(, HOOK_WALLET);

  sendInt(node_info.fd, WALLET_CONNECTION);

  //TODO: auth

  wallet_routine();

  exit(EXIT_SUCCESS);
}
