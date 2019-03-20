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
        strncpy(node.address, optarg, LEN_ADDRESS);
        flags[0] = 1;
        break;

      case 'p':
        node.port = (unsigned short)atoi(optarg);
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
  {
    strncpy(node.address, "127.0.0.1", LEN_ADDRESS);
    strncpy(wallet.address, "127.0.0.1", LEN_ADDRESS);
  }
  free(flags);

  wallet_amount = 0.0;

  fillAddressIPv4(&node_address, node.address, node.port);

  node.fd = Socket(AF_INET, SOCK_STREAM, 0);
  Connect(node.fd, (struct sockaddr *)&node_address);
  wallet = getsockNode(node.fd);
  request_t macro = WALLET_CONNECTION;
  Write(node.fd, &macro, sizeof(macro));

  //TODO: auth
  int response = 0;

  if (!(Read(node.fd, &response, sizeof(response))) && response )
    wallet_routine();

  exit(EXIT_SUCCESS);
}
