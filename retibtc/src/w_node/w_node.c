#include "w_node.h"


//make a fake transaction sending src and dst eguals to me when 'mining'
static void create_transaction(int choice)
{
  float cryptocurrecy = 0.0;
  char buffer[32], c = 'n';
  int confirm = 0;
  Trns trns = NULL;

  switch(choice)
  {
    case 1:
      if(wallet_amount == 0)
      {
        printf("\nyour balance is 0. In order to make a transaction you must first have cryptocurrecy\n");
        break;
      }
      struct confirm_new_node new_conn = choose_node();
      if(new_conn.confirm == 'y')
      {
        printf("Insert a valid amount to send: \n");
        fflush(stdin);
        scanf(" %s", buffer);
        cryptocurrecy = strtof(buffer, NULL);

        if(cryptocurrecy > wallet_amount)
          fprintf(stderr, "Sending more money than your actual fund...\nRETRY\n");
        else
        {
          printf("Are you sure want to make this payment? [y]\n");
          fflush(stdin);
          scanf(" %c", &c);

          if(c == 'y')
          {
            trns = fillTransaction(wallet_info, *(new_conn.node), cryptocurrecy);

            sendInt(node_info.fd, TRANSACTION);
            Write(node_info.fd, &trns, sizeof(trns));
            printf("\nwait confirm from peer\n");
            recvInt(node_info.fd, &confirm);

            if(!confirm)
              fprintf(stderr, "Transaction not validate\n");
            else
              wallet_amount -= cryptocurrecy;
          }
          //else returning to main menu;
        }
      }
      printf("\nReturning to main menu\n");
      break;

    case 2:
      printf("How many cryptocurrecy do you want to \'mine\'?\n");
      fflush(stdin);
      scanf(" %s", buffer);
      cryptocurrecy = strtof(buffer, NULL);

      trns = fillTransaction(wallet_info, wallet_info, cryptocurrecy);

      printf("Source: %s:%hu -> Destination: %s:%hu\n", trns->src.address, trns->src.port, trns->dst.address, trns->dst.port);
      printf("%.2f, %d",trns->amount, trns->random);


      sendInt(node_info.fd, TRANSACTION);

      sendTrns(node_info.fd, trns);

      printf("\nwait confirm from peer\n");
      recvInt(node_info.fd, &confirm);

      if(!confirm)
        fprintf(stderr, "Transaction not validate, aborting operation\n");
      else
      {
        printf("New wallet amount.\n");
        wallet_amount += cryptocurrecy;
      }
      break;
      //default never reached
  }
}


static void menu_case(int choice)
{
  switch(choice)
  {
    case 1:
      create_transaction(1);
      break;
    case 2:
      create_transaction(2);
      break;
    case 5:
      printf("Cleaning and exiting\n");
      exit(EXIT_SUCCESS);
    default:
      fprintf(stderr, "Choice is not processed, retry\n");
      break;
  }
}


static void print_menu()
{
  //system("clear");
  printf("\n\nWallet address: %s:[%hu]\nWallet balance: %5.2f \n \
    1) Make an exchange\n \
    2) Buy more cryptocurrecy\n \
    5) Exit...\n \
    (press ENTER to activate)\n",\
    wallet_info.address, wallet_info.port, wallet_amount);
}


void wallet_routine()
{
  int nread = 0 , maxfd = 0;
  int request = 0;
  int choice = 0;
  char line_buffer[16];
  fd_set fset;

  /* initialize file descriptor set */
  FD_ZERO(&fset);
  maxfd = MAX(STDIN_FILENO, node_info.fd) + 1;
  while (1)
  {
    FD_ZERO(&fset);
  	FD_SET(node_info.fd, &fset); /* set for the socket */
  	FD_SET(STDIN_FILENO, &fset); /* set for the standard input */

    print_menu();
    fflush(stdin);
    while((nread = select(maxfd, &fset , NULL , NULL , NULL)) < 0);

    if(nread < 0 && errno == EINTR)
    {
      perror("select");
      exit(EXIT_FAILURE);
    }

    /*******************
      STDIN menu_case
    *******************/
  	if (FD_ISSET(STDIN_FILENO, &fset))
    {
      fgets(line_buffer, 16, stdin);
      choice = atoi(line_buffer);
      menu_case(choice);
    }

    /***************************
      Socket connections
    ***************************/
  	if (FD_ISSET(node_info.fd, &fset))
    {
      int error = recvInt(node_info.fd, &request);

      if(error)
      {
        close(node_info.fd);
        exit(EXIT_FAILURE);
      }
      else
      {
        if(request == TRANSACTION)
        {
          Trns t = (Trns) Malloc(TRNS_SIZE);
          recvTrns(node_info.fd, t);
          printf("Received new transaction from %s:%hu\n", t->dst.address, t->dst.port);
          printf("Updating new amount\n");
          sleep(2);
        }
        else
        {
          fprintf(stderr, "Request received is not known...\n");
          sleep(2);
        }
        print_menu();
      }
    }
  }
}

