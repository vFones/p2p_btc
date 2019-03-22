#include "n_node.h"
#include "../../include/blockchain.h"


/*************************
* BLOCKCHAIN & NET UTILS *
*************************/
void visitBlock(void *arg)
{
  if(arg == NULL)
    return;

  Block b = (Block)arg;

  if(!b->n_block)
  {
    fprintf(stderr,"Block [Genesis]\n");
    return;
  }
  else
  {
    Trns trns = b->info;
    fprintf(stderr,"Block [%d] <--->", b->n_block);
    visitTransaction(trns);
  }
}


Block create_block(Trns trns)
{
  Block b = (Block)Malloc(BLOCK_SIZE);

  b->info = trns;

  pthread_rwlock_rdlock(&bchain_mtx);
  b->n_block = blockchain->b_size+1;
  fprintf(stderr, "Creating block with n_block = [%d] and blockchain size [%d]\n",b->n_block, blockchain->b_size);
  pthread_rwlock_unlock(&bchain_mtx);

  b->randomtime = 5+(rand()%11);

  fprintf(stderr,"Created block\n");

  return b;
}


void spread_block(Block b, int fd)
{
  //send to everyone exept the one who sended to me (control via fd)
  node_t n;
  Tree tmp = NULL;

  //tmp = connected_node->kids;
  n = *(node_t *)tmp->info;

  request_t macro = BLOCK_SPREAD;
  //scroll list and sent to everyone
  while(tmp->kids != NULL)
  {
    if( n.fd != fd) //execpt the one who sent to me
    {
      Write(n.fd, &macro, sizeof(macro));
      fprintf(stderr, "Sending block to %s:%d\n", n.address, n.port);
      sendBlock(n.fd, b);
    }
    tmp = tmp->kids;
    n = *(node_t *)tmp->info;
  }

  /**send to the wallet if connected to me**/

  //reusing of Tree tmp and Conn_node n
  //tmp = connected_wallet->kids;
  n = *(node_t *)tmp->info;

  // info necessary to compare address of destinatary and mine wallets
  trns_t dstwallet = *(trns_t *)b->info;
  while(tmp->kids != NULL)
  {
    if(compare_by_addr(&n, &dstwallet.dst))
    {
      macro = TRANSACTION;
      Write(n.fd, &macro, sizeof(macro));
      fprintf(stderr,"Sending block to %s:%d\n", n.address, n.port);
      sendBlock(n.fd, b);
      break;
    }
    tmp = tmp->kids;
    n = *(node_t *)tmp->info;
  }
  fprintf(stderr,"Flooding ended\n");
}


/***********************
* STDIN MENU FUNCTIONS *
***********************/
void connect_to_network()
{
  use_node_t new_conn;
  request_t macro = NODE_CONNECTION;
  int canconnect = 0;
  int fd;
  // try to connect to node_n node
  // TODO: check ip address and retry
  // TODO: check port

  choose_node(&new_conn);

  pthread_rwlock_rdlock(&node_mtx);
  for(int i = 0; i <= node_list_size; i++)
  {
    if(node_list[i].fd == 0 && node_list[i].port == 0)
    {
      canconnect = 1;
      break;
    }
  }
  pthread_rwlock_unlock(&node_mtx);

  if(!canconnect)
  {
    fprintf(stderr, "No avaible space left... quitting connection");
    return;
  }


  if(new_conn.confirm == 'y')
  {
    int bsize_new_conn = 0;
    fd = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in tmp;
    int currentbchain_size = 0;

    pthread_rwlock_rdlock(&bchain_mtx);
    currentbchain_size = blockchain->b_size;
    pthread_rwlock_unlock(&bchain_mtx);

    //Block b = (Block)Malloc(BLOCK_SIZE);

    fillAddressIPv4(&tmp, new_conn.n.address, new_conn.n.port);
    new_conn.n.fd = fd;


    if(connect(fd, (struct sockaddr *)&tmp, sizeof(struct sockaddr)) != -1)
    {
      Write(fd, &macro, sizeof(macro));

      // waiting confirming in response
      fprintf(stderr, "\nAspetto un intero che è il size della SUA blochchain\n");
      Read(fd, &bsize_new_conn, sizeof(bsize_new_conn));
      fprintf(stderr, "Size received of his blockchain: %d\n", bsize_new_conn);

      fprintf(stderr,"Mando un intero che è il size della mia blockchain\n");
      Write(fd, &currentbchain_size, sizeof(currentbchain_size));
      fprintf(stderr,"Sent my blockchain size[%d]\n", currentbchain_size);


      /*************************
       **   SYNC BLOCKCHAIN   **
       ************************/

      int diff = currentbchain_size - bsize_new_conn;
      int level;
      fprintf(stderr,"Diff = my size [%d] - his size[%d] = %d\n", currentbchain_size, bsize_new_conn, diff);


      if(diff > 0) // difference positive, i got more blocks
      {
        fprintf(stderr, "Sending block starting from %d-st block of blockchain\n", (currentbchain_size - diff)+1);
        for(level = (currentbchain_size - diff)+1; level <= currentbchain_size; level++)
        {
          Block b = NULL;
          fprintf(stderr, "Searchin by level\n");

          pthread_rwlock_rdlock(&bchain_mtx);
          b = searchByLevel(blockchain, level);
          pthread_rwlock_unlock(&bchain_mtx);

          fprintf(stderr, "Sending this block %d\n",level);
          visitBlock(b);
          sendBlock(fd, b);
          fprintf(stderr, "Sent block %d\n",level);
        }
      }
      else if(diff < 0)// he got more block than me
      {
        // ( blockchain_size - diff  = livello da cui partire a mandare )
        // e.g.: blockchain_size = 15, size = 3, level to search in tree is 12
        while(diff)
        {
          Block b = (Block)Malloc(BLOCK_SIZE);
          fprintf(stderr, "Waiting for a block\n");
          recvBlock(fd, b);

          fprintf(stderr, "Received this block\n");
          visitBlock(b);

          fprintf(stderr, "\n\nThis is my blockchain BEFORE\n\n");

          pthread_rwlock_rdlock(&bchain_mtx);
          visit_tree(blockchain->genesis, visitBlock);
          pthread_rwlock_unlock(&bchain_mtx);

          fprintf(stderr, "\nAdding to mine blockchain\n");

          pthread_rwlock_wrlock(&bchain_mtx);
          addBlockToBlockchain(blockchain, b);
          pthread_rwlock_unlock(&bchain_mtx);

          fprintf(stderr, "\n\n-----BLOCKCHAIN AFTER ------- \n\n" );

          pthread_rwlock_rdlock(&bchain_mtx);
          visit_tree(blockchain->genesis, visitBlock);
          pthread_rwlock_unlock(&bchain_mtx);

          diff++;
          free(b);
          //fprintf(stderr, "Starting flooding\n");
          //spread_block(b,fd);
        }
      }
      fprintf(stderr, "*****BLOCKCHAIN SYNCED******\n");

      /*************************
       * downloaded blockchain *
       *************************/
    }
    //failed connect
    else
    {
      perror("connect_to_network");
      return;
    }
  }
  else
  {
    fprintf(stderr, "Aborting...\n");
    return;
  }

  pthread_rwlock_wrlock(&closed_flag);
  if(connection_closed)
  {
    connection_closed = 0;
    pthread_rwlock_unlock(&closed_flag);
    fprintf(stderr, "****Aborting.*****\n");
    return;
  }
  else
  {
    pthread_rwlock_unlock(&closed_flag);

    fd_open[fd] = 1;
    if(fd > max_fd)
      max_fd = fd;

    pthread_rwlock_wrlock(&node_mtx);
    for (int i = 0; i <= node_list_size; i++)
    {
      if(node_list[i].fd == 0 && node_list[i].port == 0)
      {
        node_list[node_list_size].fd = new_conn.n.fd;
        node_list[node_list_size].port = new_conn.n.port;
        strncpy(node_list[node_list_size].address, new_conn.n.address, LEN_ADDRESS);
        node_list_size++;
        break;
      }
    }
    pthread_rwlock_unlock(&node_mtx);
    fprintf(stderr, "***** Successfull connected *****\n");
  }
}


void close_connection()
{
  use_node_t node;
  choose_node(&node);
  int found = 0;
  int tmp = 0;
  if(node.confirm == 'y')
  {

    fprintf(stderr, "Searching this: \n");
    visit_node_list(node.n);

    pthread_rwlock_wrlock(&node_mtx);
    for (int i = 0; i < node_list_size; i++)
    {
      if(node_list[i].port == node.n.port &&
         !(strncmp(node_list[i].address, node.n.address, LEN_ADDRESS)))
      {
        tmp = node_list[i].fd;
        node_list[i].fd = 0;
        node_list[i].port = 0;
        memset(&node_list[i].address, 0, sizeof(node_list[i].address));
        found = 1;
        break;
      }
    }
    pthread_rwlock_unlock(&node_mtx);

    if(found == 1)
    {
      fprintf(stderr, "Found connected node with that IP:PORT\n");
      fprintf(stderr, "******Closing connection*****\n");
      fd_open[tmp]= 0;
      close(tmp);
      found = 0;
    }
    else
      fprintf(stderr, "***** Node not found ******\n");
  }
  else
    fprintf(stderr, "Aborting operation...\n");
}


/**********************
    THREAD FUNCTIONS
***********************/
void node_connection(void* arg)
{
  int fd = *((int*)arg);
  free(arg);

  node_t n;
  getpeerNode(fd, &n);
  int currentbchain_size = 0;
  int bsize_n = 0;
  int canconnect= 0;

  pthread_rwlock_rdlock(&node_mtx);
  for(int i = 0; i <= node_list_size; i++)
  {
    if( node_list[i].fd == 0 && node_list[i].port == 0)
    {
      canconnect = 1;
      break;
    }
    else if(node_list[i].port == n.port && !(strncmp(node_list[i].address, n.address, LEN_ADDRESS)))
    {
      fprintf(stderr, "Already got that node in my table.\n");
      break;
    }
  }
  pthread_rwlock_unlock(&node_mtx);

  if(!canconnect)
  {
    fd_open[fd] = 0;
    close(fd);
    fprintf(stderr, "***** got too many peers... quitting connection ******\n");
    pthread_exit(NULL);
  }
  else
  {
    fd_open[fd] = 1;
    if(fd > max_fd)
      max_fd = fd;
  }

  //sending confirm

  pthread_rwlock_rdlock(&bchain_mtx);
  currentbchain_size = blockchain->b_size;
  pthread_rwlock_unlock(&bchain_mtx);

  fprintf(stderr, "\nInvio un intero che è il size della mia blockchain\n");
  Write(fd, &currentbchain_size, sizeof(currentbchain_size));
  fprintf(stderr, "Sent size of my blockchain: [%d]\n", currentbchain_size);

  fprintf(stderr, "Ricevo size della SUA blockchain\n");
  Read(fd, &bsize_n, sizeof(bsize_n));
  fprintf(stderr, "Received his blockchain size: [%d]\n", bsize_n);


  int diff = currentbchain_size - bsize_n;
  fprintf(stderr,"Diff = blockchain size [%d] - HIS blockchain size[%d] = %d\n", currentbchain_size, bsize_n, diff);

  int level;

  //if my blockchain is bigger send
  if(diff > 0)
  {
    int starting = currentbchain_size - diff;
    fprintf(stderr,"Sending block starting from %d-st block of blockchain\n", starting);
    for(level = starting+1; level <= diff; level++)
    {
      Block b = NULL;
      fprintf(stderr, "Searchin by level\n");

      pthread_rwlock_rdlock(&bchain_mtx);
      b = searchByLevel(blockchain, level);
      pthread_rwlock_unlock(&bchain_mtx);

      fprintf(stderr, "found this block\n");
      visitBlock(b);

      fprintf(stderr, "Sending this block  %d\n", level);
      visitBlock(b);
      sendBlock(fd, b);
      fprintf(stderr, "Sent block %d\n", level);
    }
  }
  else if(diff < 0) // i must receive
  {
    fprintf(stderr, "Receiving block \n");
    while(diff)
    {
      Block b = (Block)Malloc(BLOCK_SIZE);
      fprintf(stderr, "Ricevo blocco\n");
      recvBlock(fd, b);
        fprintf(stderr, "ricevo questo blocco\n");
      visitBlock(b);

        fprintf(stderr, "\n\n--------- BLOCKCHAIN BEFORE *********\n\n");

        pthread_rwlock_rdlock(&bchain_mtx);
        visit_tree(blockchain->genesis, visitBlock);
        pthread_rwlock_unlock(&bchain_mtx);

        fprintf(stderr ,"Adding to mine blockchain\n");

      pthread_rwlock_wrlock(&bchain_mtx);
      addBlockToBlockchain(blockchain, b);
      pthread_rwlock_unlock(&bchain_mtx);

        fprintf(stderr, "\n\n-----BLOCKCHAIN AFTER ------- \n\n" );

        pthread_rwlock_rdlock(&bchain_mtx);
        visit_tree(blockchain->genesis, visitBlock);
        pthread_rwlock_unlock(&bchain_mtx);

        diff++;
        free(b);
      //printf("Starting flooding\n");
      //spread_block(b,fd);
    }
  }
  fprintf(stderr, "***** BLOCKCHAIN SYNCED ******\n");


  pthread_rwlock_wrlock(&closed_flag);
  if(connection_closed)
  {
    connection_closed = 0;
    pthread_rwlock_unlock(&closed_flag);
    fprintf(stderr, "***** Exiting from thread that manages connections. *****\n");
    fprintf(stderr, "**** Aborting. *****\n");
    return;
    //pthread_exit(NULL);
  }
  else
  {
    pthread_rwlock_unlock(&closed_flag);

    fd_open[fd] = 1;
    if(fd > max_fd)
      max_fd = fd;

    pthread_rwlock_wrlock(&node_mtx);
    for (int i = 0; i <= node_list_size; i++)
    {
      if(node_list[i].port == n.port && !(strncmp(node_list[i].address, n.address, LEN_ADDRESS)))
      {
        fprintf(stderr, "***** Already got that node in my table *******\n");
        break;
      }
      else if(node_list[i].fd == 0 && node_list[i].port == 0)
      {
        node_list[node_list_size].fd = n.fd;
        node_list[node_list_size].port = n.port;
        strncpy(node_list[node_list_size].address, n.address, LEN_ADDRESS);
        node_list_size++;
        break;
      }
    }
    pthread_rwlock_unlock(&node_mtx);
    fprintf(stderr, "***** Exiting from thread that manages connections. *****\n");
    return;
    //pthread_exit(NULL);
  }
}


void wallet_connection(void *arg)
{
  int fd = *((int*)arg);
  free(arg);
  fprintf(stderr, "********* getting fd new wallet connection \n *********");

  int canconnect = 0;
  node_t n;
  getpeerNode(fd, &n);


  pthread_rwlock_rdlock(&node_mtx);
  for(int i = 0; i <= wallet_list_size; i++)
  {
    if( wallet_list[i].fd == 0 && wallet_list[i].port == 0)
    {
      canconnect = 1;
      break;
    }
    else if(wallet_list[i].port == n.port && !(strncmp(wallet_list[i].address, n.address, LEN_ADDRESS)))
      break;
  }
  pthread_rwlock_unlock(&node_mtx);

  if(!canconnect)
  {
    fd_open[fd] = 0;
    close(fd);
    fprintf(stderr, "***** Can't connect right now... quitting connection ******\n");
    return;
    //pthread_exit(NULL);
  }
  else
  {
    fd_open[fd] = 1;
    if(fd > max_fd)
      max_fd = fd;
  }

  int confirm = 1;
  Write(fd, &confirm, sizeof(confirm));

  pthread_rwlock_wrlock(&closed_flag);
  if(connection_closed)
  {
    connection_closed = 0;
    pthread_rwlock_unlock(&closed_flag);
    fprintf(stderr, "*****Exiting from thread that manages connections.*****\n");
    fprintf(stderr, "****Aborting.*****\n");
    return;
    //pthread_exit(NULL);
  }
  else
  {
    pthread_rwlock_unlock(&closed_flag);

    fd_open[fd] = 1;
    if(fd > max_fd)
      max_fd = fd;

    pthread_rwlock_wrlock(&node_mtx);
    for (int i = 0; i <= wallet_list_size; i++)
    {
      if(wallet_list[i].port == n.port && !(strncmp(wallet_list[i].address, n.address, LEN_ADDRESS)))
      {
        fprintf(stderr, "Already got that wallet in my table.\n");
        pthread_exit(NULL);
      }
      if( wallet_list[i].fd == 0 && wallet_list[i].port == 0)
      {
        wallet_list[wallet_list_size].fd = n.fd;
        wallet_list[wallet_list_size].port = n.port;
        strncpy(wallet_list[wallet_list_size].address, n.address, LEN_ADDRESS);
        wallet_list_size++;
        break;
      }
    }
    pthread_rwlock_unlock(&node_mtx);
    fprintf(stderr, "*****Exiting from thread that manages connections.*****\n");
    return;
    //pthread_exit(NULL);
  }
}


void receive_transaction(void *arg)
{
  int fd = *((int*)arg);
  free(arg);

  // receiv transaction from w_node
  Trns trns = (Trns)Malloc(TRNS_SIZE);
  Read(fd, trns, TRNS_SIZE);

  fprintf(stderr,"package from %s:%d\n", trns->src.address, trns->src.port);
  // send confirm
  int confirm = 1;
  Write(fd, &confirm, sizeof(confirm));

  //creating block with transaction
  Block b = create_block(trns);

  //waiting rand sec
  fprintf(stderr,"Waiting %d sec \n", b->randomtime);
  //sleep((unsigned int)b->randomtime);

  fprintf(stderr, "Block[%d]\n",b->n_block);

  //if created block is with already old(received another block)
  pthread_rwlock_rdlock(&bchain_mtx);
  if (b->n_block <= blockchain->b_size)
  {
    pthread_rwlock_unlock(&bchain_mtx);
    fprintf(stderr,"Recreating block\n");
    b = create_block(trns);
  }//recreate
  else
    pthread_rwlock_unlock(&bchain_mtx);

  fprintf(stderr,"Adding to blockchain\n");

  pthread_rwlock_wrlock(&bchain_mtx);
  addBlockToBlockchain(blockchain, b);
  pthread_rwlock_unlock(&bchain_mtx);

  fprintf(stderr,"Added to blockchain\n\n");

  //spread_block();
  //pthread_exit(NULL);
}


void receive_block(void *arg)
{
  int fd = *((int*)arg);
  free(arg);
  // received from a thread
  Block b = NULL;

  recvBlock(fd, b);
  fprintf(stderr,"Received new block\n");

  pthread_rwlock_wrlock(&bchain_mtx);
  addBlockToBlockchain(blockchain, b);
  pthread_rwlock_unlock(&bchain_mtx);

  //spread to others:
  fprintf(stderr,"Start flooding\n");
  spread_block(b, fd);

  //pthread_exit(NULL);
}


/******************
   N_NODE ROUTINE
******************/
void n_routine()
{
  srand(time(NULL));

  //sig_action and masks
  struct sigaction sig_act;
  sig_act.sa_handler = sig_handler;
  sig_act.sa_flags = 0;

  sigset_t new_mask, old_mask;
  sigemptyset(&sig_act.sa_mask);
  sigaction(SIGINT, &sig_act, NULL);

  sigemptyset(&new_mask);
  sigemptyset(&old_mask);
  sigaddset(&new_mask, SIGINT);
  sigprocmask(SIG_SETMASK, NULL, &old_mask);

  // main FDs to monitor
  int list_fd = 0;
  list_fd = Socket(AF_INET, SOCK_STREAM, 0);

  // socket option with SO_REUSEADDR
  int opt_value = 1;
  setsockopt(list_fd, SOL_SOCKET, SO_REUSEADDR, &opt_value, sizeof(int));

  struct sockaddr_in my_server_addr;
  fillAddressIPv4(&my_server_addr, NULL, service_port);

  // bindin address:port to socket fd and setting a backlog
  Bind(list_fd, (struct sockaddr *)&my_server_addr);
  Listen(list_fd, BACKLOG);

  //mutex dinamically allocated
  pthread_rwlock_init(&bchain_mtx, NULL);
  pthread_rwlockattr_init(&bchain_mtx_attr);
  blockchain = create_blockchain();

  pthread_rwlock_init(&node_mtx, NULL);
  pthread_rwlockattr_init(&node_mtx_attr);
  node_list_size = 0;
  wallet_list_size = 0;

  pthread_rwlock_init(&closed_flag, NULL);
  pthread_rwlockattr_init(&closed_flag_attr);
  connection_closed = 0;

  node_list = (node_t *)calloc(BACKLOG, sizeof(node_t));
  wallet_list = (node_t *)calloc(BACKLOG, sizeof(node_t));

  // creating blockchain;

  // used for select management and monitor
  int i_fd = 0, n_ready = 0;
  fd_set fdset; //fdset to fill in select while
  int accept_fd = 0;

  int response = 0;
  request_t request = 0;

  // thread stuff
  pthread_t *tid = (pthread_t *)Malloc(BACKLOG);
  int tid_index = 0;

  // used for switch case men\nu
  int choice = 0;
  char line_buffer[16];

  //settin max_fd as list_fd and monitoring that on fd_open table
  pthread_mutex_init(&fd_mtx, NULL);
  fd_open = (int *)calloc(FD_SETSIZE, sizeof(int));

  // setting list_fd as max
  fd_open[list_fd] = 1;
  max_fd = list_fd;

  while (1)
  {
    // critic section used to control flag
    sigprocmask(SIG_BLOCK, &new_mask, NULL);

    if(exit_flag == 1)
      break;

    FD_ZERO(&fdset);
    FD_SET(STDIN_FILENO, &fdset);
    FD_SET(list_fd, &fdset);
    //re update fdset with fd_open monitor table
    for (i_fd = 0; i_fd <= max_fd; i_fd++)
      if(fd_open[i_fd])
        FD_SET(i_fd, &fdset);


    //system("clear");

    fprintf(stderr,"\n* * * * * Connected N_NODE * * * * *\n");
    pthread_rwlock_rdlock(&node_mtx);
    for(int i=0; i < node_list_size; i++)
      visit_node_list(node_list[i]);
    //visit_tree(connected_node, visitConnectedNode);
    pthread_rwlock_unlock(&node_mtx);

    fprintf(stderr,"\n");

    fprintf(stderr,"* * * * * Connected W_NODE * * * * *\n");
    pthread_rwlock_rdlock(&node_mtx);
    for(int i=0; i < wallet_list_size; i++)
      visit_wallet_list(wallet_list[i]);
    //visit_tree(cofnnected_wallet, visitConnectedWallet);
    pthread_rwlock_unlock(&node_mtx);

    fprintf(stderr,"\n");

    fprintf(stderr,"* * * * * BLOCKCHAIN * * * * *\n");
    pthread_rwlock_rdlock(&bchain_mtx);
    visit_tree(blockchain->genesis, visitBlock);
    pthread_rwlock_unlock(&bchain_mtx);

    fprintf(stderr,"\n");

    fprintf(stderr,"\tChoose:\n1) connect to peers;\n2) disconnect from peer\n5) quit\n");
    fprintf(stderr, ">_ ");
    fflush(stdin);
    n_ready = pselect(max_fd + 1, &fdset, NULL, NULL, NULL, &old_mask); //reset

    if(n_ready < 0 && errno == EINTR)
    {
      sigprocmask(SIG_UNBLOCK, &new_mask, NULL); // unlock if signal's != SIGINT
      continue;
    }


    if(n_ready < 0)
    {
      perror("select error");
      exit(1);
    }


    sigprocmask(SIG_UNBLOCK, &new_mask, NULL);
    // exit critic section unlocking signal


    /*********************
        STDIN menu_case
    **********************/
    if(FD_ISSET(STDIN_FILENO, &fdset))
    {
      n_ready--;
      fflush(stdin);
      fgets(line_buffer, 16, stdin);
      choice = atoi(line_buffer);
      switch(choice)
      {
        case 1:
          connect_to_network();
          break;
        case 2:
          close_connection();
          break;
        case 5:
          printf("Cleaning and exiting\n");
          exit_flag = 1;
          break;
          //exit(EXIT_SUCCESS);
        default:
          fprintf(stderr, "Choice is not processed: wrong input\n");
          break;
      }
    }

    /***************************
        Socket connections
    ***************************/

    if(FD_ISSET(list_fd, &fdset))
    {
      n_ready--; //accepting after new connection
      int keepalive = 1;
      if((accept_fd = accept(list_fd, NULL, NULL)) < 0)
      {
        perror("accept");
        exit(EXIT_FAILURE);
      }
      setsockopt(accept_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));

      // setting fd ready for reading
      fd_open[accept_fd] = 1;

      //calculating new max
      if(max_fd < accept_fd)
        max_fd = accept_fd;
    }
    i_fd = list_fd;
    tid_index = 0;
    while (n_ready)
    {
      // selecting i_fd to serve
      i_fd++;
      if(!fd_open[i_fd])
        continue;

      // if socket wakeup
      if(FD_ISSET(i_fd, &fdset))
      {
        n_ready--;

        response = Read(i_fd, &request, sizeof(request));

        /************************************
         * if closed because EOF while Read;
         ************************************/
        if(response != 0)
        {
          pthread_rwlock_wrlock(&closed_flag);
          connection_closed = 1;
          pthread_rwlock_unlock(&closed_flag);

          fprintf(stderr, "******* Peer has closed connection ******\n");
          //closing
          fd_open[i_fd] = 0;
          close(i_fd);

          pthread_rwlock_wrlock(&node_mtx);
          for(int i = 0; i < node_list_size; i++)
          {
            if( node_list[i].fd == i_fd )
            {
              node_list[i].fd = 0;
              node_list[i].port = 0;
              memset(&node_list[i].address, 0, sizeof(node_list[i].address));
              break;
            }
          }

          for(int i = 0; i < wallet_list_size; i++)
          {
            if( wallet_list[i].fd == i_fd )
            {
              wallet_list[i].fd = 0;
              wallet_list[i].port = 0;
              memset(&wallet_list[i].address, 0, sizeof(wallet_list[i].address));
              break;
            }
          }
          pthread_rwlock_unlock(&node_mtx);

          pthread_rwlock_wrlock(&closed_flag);
          connection_closed = 0;
          pthread_rwlock_unlock(&closed_flag);

          //updating max fd with the last open in fd_open
          if(max_fd == i_fd)
          {
            while (fd_open[--i_fd] == 0);
            max_fd = i_fd;
            break;
          }
          continue;
        }
        /*****************/

        int *arg = (int *)Malloc(sizeof(int));
        *arg = i_fd;
        //request received correctly, switching between cases
        switch (request)
        {
          case NODE_CONNECTION:
            fprintf(stderr,"********* Creating thread for node_connection *********\n");
            node_connection(arg);
            //pthread_create(&tid[tid_index], NULL, node_connection, arg);
            break;
          case WALLET_CONNECTION:
            fprintf(stderr,"******** Creating thread for wallet_connection *********\n");
            wallet_connection(arg);
            //pthread_create(&tid[tid_index], NULL, wallet_connection, arg);
            break;
          case TRANSACTION:
            fprintf(stderr,"******* Creating thread for receive_transaction ********\n");
            receive_transaction(arg);
            //pthread_create(&tid[tid_index], NULL, receive_transaction, arg);
            break;
          case BLOCK_SPREAD:
            fprintf(stderr,"******* Creating thread for receive_block (spreading) *******\n");
            receive_transaction(arg);
            //pthread_create(&tid[tid_index], NULL, receive_block, arg);
            break;
          default:
            fprintf(stderr, "Node: request received is not correct\n");
            break;
        }
        //tid_index++;
      }
    }
    //not waiting threads so I can exit with C-c and kill everybody
  }

  for(int j=0; j < tid_index;  j++)
    pthread_kill(tid[j], SIGKILL);

  fprintf(stderr, "Closing node\n");
  for (int i = 0; i <= max_fd; i++)
  {
    if (fd_open[i])
    {
      fprintf(stderr, "Closing fd_open[i_fd]: %d\n", i);
      close(i);
    }
  }

  //free (blockchain);

  free(fd_open);
  free(node_list);
  free(wallet_list);

  pthread_mutex_destroy(&fd_mtx);
  pthread_rwlock_destroy(&bchain_mtx);
  pthread_rwlockattr_destroy(&bchain_mtx_attr);
  pthread_rwlock_destroy(&node_mtx);
  pthread_rwlockattr_destroy(&node_mtx_attr);
  pthread_rwlock_destroy(&closed_flag);
  pthread_rwlockattr_destroy(&closed_flag_attr);

  exit(EXIT_SUCCESS);
}
