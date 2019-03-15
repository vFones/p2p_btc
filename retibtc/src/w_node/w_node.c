#include "w_node.h"


//make a fake transaction, sending src and dst eguals to me.
static void create_transaction(int choice)
{
  float cryptocurrecy = 0.0;
  char buffer[32], c = 'n';
  int confirm = 0;
  struct transaction trns;

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
            trns = fillTransaction(wallet_info, wallet_info, cryptocurrecy);

            sendInt(node_info.fd, TRANSACTION);
            sendTrns(node_info.fd, trns);
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


      trns = fillTransaction(wallet_info, wallet_info, cryptocurrecy );

      sendInt(node_info.fd, TRANSACTION);
      sendTrns(node_info.fd, trns);

      printf("\nwait confirm from peer\n");
      recvInt(node_info.fd, &confirm);

      if(!confirm)
        fprintf(stderr, "Transaction not validate, aborting operation\n");
      else
        wallet_amount += cryptocurrecy;

      break;

    default:
      //never reached
      break;
  }


}


static void menu_case(int choice)
{
  switch(choice)
  {
    case 1:
      //request_amount();
      break;
    case 2:
      create_transaction(1);
      break;
    case 3:
      create_transaction(2);
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


static void print_menu()
{
  //system("clear");
  printf("\n\nWallet address: %s:[%hu]\nWallet balance: %5.2f \n \
    1) Request amount of cryptocurrecy.\n \
    2) Make an exchange\n \
    3) Buy more cryptocurrecy\n \
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
      fflush(stdin);
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
      if(error != 0)
      {
        fprintf(stderr,"Socket error: connection rejected from peer.\n");
        close(node_info.fd);
        exit(EXIT_FAILURE);
      }
      else
      {
        switch(request)
        {
          case TRANSACTION:
            //receive_transaction();
            break;

          default:
            fprintf(stderr, "Request received is not known...\n");
            print_menu();
            break;
        }
      }
    }
  }
  return;
}




//USER MENU STUFF
//
//request amount (calculate wall_amount from all transaction received)
// void request_amount()
// {
//   int sizeoftrns = 0;
//   Trns trns = NULL;
//   float tmp_amount = 0.0;
//
//   send_short(peerfd, W_BALANCE);
//   recv_int(peerfd, &sizeoftrns);
//
//   if(sizeoftrns == 0)
//   {
//     printf("\nyou have not yet made / received transactions\n");
//     free(trns);
//     return;
//   }
//
//   List list = create_list();
//
//   for(int i = 0; i < sizeoftrns; i++)
//   {
//     trns = (Trns)obj_malloc(TRNS);
//     recv_trns(peerfd, trns);
//     add_to_list(list, trns);
//   }
//
//   printf("Calculating refreshed balance...\n\n");
//   for(int i = 1; i <= list->count; i++)
//   {
//     trns = search_by_index(list, i);
//     if(
//       compare_net_ent(&myent, &(trns->src)) &&
//       !compare_net_ent(&myent, &(trns->dst))
//     )
//     {
//       tmp_amount -= trns->amount;
//       printf("\t-%5.2f\n", trns->amount);
//     }
//     else
//     {
//       tmp_amount += trns->amount;
//       printf("\t+%5.2f\n", trns->amount);
//     }
//   }
//
//   wallet_amount = tmp_amount;
//   empty_list(list);
//   free(list);
//   free(trns);
//   return;
// }
//
// //create a transaction and send it to peer


//
// //
// //PEER STUFF
// //
// //receive transaction and add it to yours amount
// void receive_transaction()
// {
//   struct s_trns trns;
//   recv_trns(peerfd, &trns);
//   printf("\nReceived transaction! You have received %5.2f from [%s:%d]\n", \
//     trns.amount, trns.src.addr, trns.src.port);
//   printf("\nNew balance now is:\n");
//   request_amount();
//   visit_trns((void*)&trns);
// }
//
// void hook2net()
// {
//   recv_net_ent(servfd, &peer);
//
//   if(!peer.port)
//   {
//     fprintf(stderr,"Peer not valid, aborting\n");
//     exit(EXIT_FAILURE);
//   }
//   close(servfd);
//
//   printf("Trying to connect to %s:%d\n", peer.addr, peer.port);
//
//   fill_address(&peeraddr, AF_INET, peer.addr, peer.port);
//   peerfd = Socket(AF_INET, SOCK_STREAM, 0);
//   Connect(peerfd, (struct sockaddr *)&peeraddr);
//
//   send_short(peerfd, HOOK_W2P);
//   short response;
//   recv_short(peerfd, &response);
//   if(!response)
//   {
//     fprintf(stderr, "Peer refused my connection\n");
//     exit(EXIT_FAILURE);
//   }
//   else
//   {
//     getsock_net_ent(peerfd, &myent);
//     printf("Correctly hooked\n");
//   }
// }
//
// //in case of peer crash, re hook to network
// void re_hook()
// {
//   server_authentication();
//   send_short(servfd, RE_HOOK_WALLET);
//   send_net_ent(servfd, &peer);
//   hook2net();
// }
//
// //authenticate with server for hooking procedures.
// void server_authentication()
// {
//   servfd = Socket(AF_INET, SOCK_STREAM, 0);
//   Connect(servfd, (struct sockaddr *)&servaddr);
//   if(!sha_auth(servfd, pwd)) // wrong password
//   {
//     fprintf(stderr, "\nwrong password!\n");
//     usage();
//   }
// }
