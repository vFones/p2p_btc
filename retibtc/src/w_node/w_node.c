#include "w_node.h"


//make a fake transaction sending src and dst eguals to me when 'mining'
static void create_transaction(int choice)
{
  float cryptocurrecy = 0.0;
  char buffer[32], c = 'n';
  int confirm = 0;
  Trns t = (Trns)Malloc(TRNS_SIZE);
  request_t macro = TRANSACTION;

  switch(choice)
  {
    case 1:
      if(wallet_amount == 0)
      {
        fprintf(stderr, "\nyour balance is 0. In order to make a transaction you must first have cryptocurrecy\n");
        break;
      }
      use_node_t new_conn;
      choose_node(&new_conn);
      if(new_conn.confirm == 'y')
      {
        fprintf(stderr,"Insert a valid amount to send: \n");
        fflush(stdin);
        scanf(" %s", buffer);
        cryptocurrecy = strtof(buffer, NULL);

        if(cryptocurrecy > wallet_amount)
          fprintf(stderr, "Cannot send more than your actual funds.\n**** Retry *****\n");
        else
        {
          fprintf(stderr, "Are you sure want to make this payment? [y]\n");
          fflush(stdin);
          scanf(" %c", &c);

          if(c == 'y')
          {
            t = fillTransaction(wallet, new_conn.n, cryptocurrecy);
            Write(node.fd, &macro, sizeof(macro));
            Write(node.fd, t, TRNS_SIZE);
            fprintf(stderr,"\nwait confirm from peer\n");
            Read(node.fd, &confirm, sizeof(confirm));

            if(!confirm)
              fprintf(stderr, "Transaction not validated.\n");
            else
              wallet_amount -= cryptocurrecy;
          }
          //else returning to main menu;
        }
      }
      fprintf(stderr,"\nReturning to main menu.\n");
      break;

    case 2:
      fprintf(stderr,"How many cryptocurrecy do you want to \'mine\'?\n");
      fflush(stdin);
      scanf(" %s", buffer);
      cryptocurrecy = strtof(buffer, NULL);

      t = fillTransaction(wallet, wallet, cryptocurrecy);

      visitTransaction(t);

      Write(node.fd, &macro, sizeof(macro));
      Write(node.fd, t, TRNS_SIZE);

      fprintf(stderr,"\nwait confirm from peer...\n");
      Read(node.fd, &confirm, sizeof(confirm));

      if(!confirm)
        fprintf(stderr, "Transaction not validated, aborting operation.\n");
      else
      {
        fprintf(stderr,"Refreshing wallet amount.\n");
        wallet_amount += cryptocurrecy;
      }
      break;
      //default never reached
  }
}


static void print_menu()
{
  //system("clear");
  fprintf(stderr,"\n\nWallet address: %s:[%d]\nWallet balance: %5.2f \n \
    1) Make an exchange\n \
    2) Buy more cryptocurrecy\n \
    5) Exit...\n \
    (press ENTER to activate)\n",\
    wallet.address, wallet.port, wallet_amount);
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
  maxfd = MAX(STDIN_FILENO, node.fd) + 1;
  while (1)
  {
    FD_ZERO(&fset);
  	FD_SET(node.fd, &fset); /* set for the socket */
  	FD_SET(STDIN_FILENO, &fset); /* set for the standard input */

    print_menu();
    nread = select(maxfd, &fset , NULL , NULL , NULL);

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
  	  fprintf(stderr,">_ ");
      fgets(line_buffer, 16, stdin);
      choice = atoi(line_buffer);
      switch(choice)
      {
        case 1:
          create_transaction(1);
          break;
        case 2:
          create_transaction(2);
          break;
        case 5:
          fprintf(stderr,"Cleaning and exiting\n");
          exit(EXIT_SUCCESS);
        default:
          fprintf(stderr, "Error: can't recognize request. Retry.\n");
          break;
      }
    }

    /***************************
      Socket connections
    ***************************/
  	if (FD_ISSET(node.fd, &fset))
    {
  	  int error = Read(node.fd, &request, sizeof(request));

      if(error)
      {
        close(node.fd);
        exit(EXIT_FAILURE);
      }
      else
      {
        if(request == TRANSACTION)
        {
          trns_t t;
          Read(node.fd, &t, sizeof(t));
          fprintf(stderr,"\n\n * Received new transaction from %s:%d\n", t.dst.address, t.dst.port);
          fprintf(stderr,"* Updating new amount\n");
          // TODO:
          sleep(2);
        }
        else
        {
          fprintf(stderr, "\n\n * Request received is not known...\n");
          sleep(2);
        }
        print_menu();
      }
    }
  }
}

