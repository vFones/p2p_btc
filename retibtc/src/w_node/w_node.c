#include "w_node.h"

static void menu_case(int choice)
{
  switch(choice)
  {
    case 1:
      //request_amount();
      break;
    case 2:
      //send_coin();
      break;
    case 3:
      //add_coin();
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
  printf("\n\nWallet address: %s:[%d]\nWallet balance: %5.2f \n \
    1) Request amount of ViTC.\n \
    2) Make an exchange\n \
    3) Buy more ViCoin\n \
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
// void send_coin()
// {
//   printf("\nrefresh balance\n");
//   request_amount();
//   if(wallet_amount == 0)
//   {
//     printf("\nyour balance is 0. In order to make a transaction you must first buy ViTCoins!!\n");
//     return;
//   }
//   char buffer[32], c = 'n';
//   float vtc2send = 0.0;
//   struct s_net_ent dst;
//   Trns trns = (Trns) obj_malloc(TRNS);
//   short confirm = 0;
//
//   while (c != 'y')
//   {
//     printf("\nInsert a valid IPv4 address: ");
//     fflush(stdin);
//     scanf(" %s", buffer);
//     strncpy(dst.addr, buffer, LEN_ADDRESS);
//
//     printf("Insert a valid port address: ");
//     fflush(stdin);
//     scanf(" %hud", &dst.port);
//
//     printf("Insert a valid amount to send: ");
//
//     fflush(stdin);
//     scanf(" %s", buffer);
//     vtc2send = strtof(buffer, NULL);
//
//     if(vtc2send > wallet_amount)
//       fprintf(stderr, "Sending more money than your actual fund...\nRETRY\n");
//     else
//     {
//       printf("are those info correct? [y]\n");
//       fflush(stdin);
//       scanf(" %c",&c);
//     }
//   }
//   wallet_amount = wallet_amount - vtc2send;
//
//   printf("Are you sure want to make this payment? [y]\n");
//   fflush(stdin);
//   scanf(" %c",&c);
//
//   if(c == 'y')
//   {
//     char *timestamp = gen_time_stamp();
//     trns = create_transaction(timestamp, vtc2send, myent, dst);
//
//     send_short(peerfd, W_TRANSACTION);
//
//     send_trns(peerfd, trns);
//
//     printf("Waiting confirm from peer...\n");
//
//     recv_short(peerfd, &confirm);
//
//     if(!confirm)
//       fprintf(stderr, "Transaction not validate, aborting operation\n");
//   }
//
//   printf("\nNew balance now is:\n");
//   request_amount();
//   printf("\nReturning to main menu\n");
//   free(trns);
// }
//
// //make a fake transaction, sending src and dst eguals to me.
// void add_coin()
// {
//   float vtc2buy = 0.0;
//   char buffer[32];
//   short confirm = 0;
//
//   printf("How many ViTCoin do you want to \'mine\'?\n");
//   fflush(stdin);
//   scanf(" %s", buffer);
//   vtc2buy = strtof(buffer, NULL);
//
//   char *timestamp = gen_time_stamp();
//   Trns trns = create_transaction(timestamp, vtc2buy, myent, myent);
//   send_short(peerfd, W_TRANSACTION);
//   send_trns(peerfd, trns);
//
//   printf("\nwait confirm from peer\n");
//   for(int i = 0; i<3; i++)
//   {
//     sleep(1);
//     printf("\n.");
//   }
//   recv_short(peerfd, &confirm);
//
//   if(!confirm)
//     fprintf(stderr, "Transaction not validate, aborting operation\n");
//
//   printf("\nNew balance now is:\n");
//   request_amount();
//   free(trns);
// }
//
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
