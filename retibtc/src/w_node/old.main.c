#include "wallet.h"

int main(int argc, char **argv)
{
  int opt;


  int *flags = (int *)calloc(4, sizeof(int));

  while ( (opt = getopt(argc, argv, "b:p:h:")) != -1)
  {
    switch(opt)
    {
      case 'b':
        strncpy(server.addr, optarg, LEN_ADDRESS);
        flags[0] = 1;
        break;

      case 'p':
        server.port = (unsigned short)atoi(optarg);
        flags[1] = 1;
        break;

      case 'h':
        flags[2] = 1;
        strcpy(pwd, optarg);
        break;

      default:
        flags[3] = 1;
        usage();
        break;
    }
  }

  if( !flags[2] || flags[3] )
    usage();
  if(!flags[0])
    strncpy(server.addr, "127.0.0.1", LEN_ADDRESS);
  if(!flags[1])
    server.port = 7777;

  free(flags);

  wallet_amount = 0.0;

  printViTCmsg("\nWELCOME IN YOUR ViTCoin Wallet \n");

  fill_address(&servaddr, AF_INET, server.addr, server.port);
  server_authentication();
  printf("\nAuthenticated, receiving peer 2 connect...\n\n");

  send_short(servfd, HOOK_WALLET);
  hook2net();
  wallet_routine();

  exit(EXIT_SUCCESS);
}


void wallet_routine()
{
  int nread = 0 , maxfd = 0;
  short request = 0;
  int choice = 0;
  char line_buffer[16];
  fd_set fset;

  /* initialize file descriptor set */
  FD_ZERO(&fset);
  maxfd = MAX(STDIN_FILENO, peerfd) + 1;
  while (1)
  {
    FD_ZERO(&fset);
  	FD_SET(peerfd, &fset); /* set for the socket */
  	FD_SET(STDIN_FILENO, &fset); /* set for the standard input */

    print_menu();
    fflush(stdin);
    while((nread = select(maxfd, &fset , NULL , NULL , NULL)) < 0 && \
      errno == EINTR);

    if(nread < 0)
    {
      perror("select");
      exit(EXIT_FAILURE);
    }

  	if (FD_ISSET(STDIN_FILENO, &fset))
    {
      fflush(stdin);
      fgets(line_buffer, 16, stdin);
      choice = atoi(line_buffer);

      // sscanf(line_buffer, "%d", &choice);
      menu_case(choice);
    }
  	if (FD_ISSET(peerfd, &fset))
    {
      int error = recv_short(peerfd, &request);
      if(error != 0)
      {
        fprintf(stderr,"Socket error: connection rejected from peer.\n");
        close(peerfd);
        re_hook();
      }
      else
        request_case(request);
    }
  }
  return;
}


void menu_case(int choice)
{
  switch(choice)
  {
    case 1:
      request_amount();
      break;
    case 2:
      send_coin();
      break;
    case 3:
      add_coin();
      break;
    case 5:
      printf("Cleaning and exiting\n");
      exit(EXIT_SUCCESS);
      break;
    default:
      fprintf(stderr, "Choice is not processed, retry\n");
      break;
  }
}


void request_case(short request)
{
  switch(request)
  {
    case TRANSACTION:
      receive_transaction();
      break;

    default:
      fprintf(stderr, "Request received is not correct\n");
      print_menu();
      break;
  }
}


void print_menu()
{
  //system("clear");

  printf("\n\nWALLET INFO: %s:[%d]\nWALLET BALANCE: %5.2f \n", \
    myent.addr, myent.port, wallet_amount);
  printf("1) Request amount of ViTC.\n");
  printf("2) Make an exchange\n");
  printf("3) Buy more ViCoin\n" );
  printf("5) Exit...\n");
  printViTCmsg("\n\tCHOSE AN OPERATION\n");
  printf("(press ENTER to activate)\n");
}


void usage()
{
  fprintf(stderr, "Usage: ./wallet (-b server -p server_port) -h token \n");
  exit(EXIT_FAILURE);
}
