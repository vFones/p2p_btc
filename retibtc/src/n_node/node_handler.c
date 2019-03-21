#include "n_node.h"
#include "../../include/blockchain.h"




//TODO: move file in a pragmatic way

//TODO: CLEAN printf

//TODO: SINCRONIZATION FD AND BLOCKCHAIN

/*************************
* BLOCKCHAIN & NET UTILS *
*************************/
void visitBlock(void *arg)
{
  if(arg == NULL)
    return;

  Block b = (Block)arg;

  if(!b->n_block)
    printf("\tBlock [GENESIS]\n");
  else
  {
    trns_t trns = *(trns_t *)b->info;
    printf("Block[%d] --> [%s:%hu -> %s:%hu] [%0.2f BTC] [%d] \n", \
      b->n_block, trns.src.address, trns.src.port, \
      trns.dst.address, trns.dst.port, trns.amount, trns.random);
  }
}


Block create_block(trns_t trns)
{
  Block b = (Block)Malloc(BLOCK_SIZE);
  b->info = &trns;

  pthread_rwlock_rdlock(&bchain_mtx);
  b->n_block = blockchain->b_size+1;
  printf("Creating block with n_block = [%d] and blockchain size [%d]\n",b->n_block, blockchain->b_size);
  pthread_rwlock_unlock(&bchain_mtx);

  b->randomtime = 5+(rand()%11);

  printf("Created block\n");

  return b;
}


void spread_block(Block b, int fd)
{
  //send to everyone exept the one who sended to me (control via fd)
  node_t n;
  Tree tmp = NULL;

  tmp = connected_node->kids;
  n = *(node_t *)tmp->info;

  request_t macro = BLOCK_SPREAD;
  //scroll list and sent to everyone
  while(tmp->kids != NULL)
  {
    if( n.fd != fd) //execpt the one who sent to me
    {
      Write(n.fd, &macro, sizeof(macro));
      printf("Sending block to %s:%hu\n", n.address, n.port);
      sendBlock(n.fd, b);
    }
    tmp = tmp->kids;
    n = *(node_t *)tmp->info;
  }

  /**send to the wallet if connected to me**/

  //reusing of Tree tmp and Conn_node n
  tmp = connected_wallet->kids;
  n = *(node_t *)tmp->info;

  // info necessary to compare address of destinatary and mine wallets
  trns_t dstwallet = *(trns_t *)b->info;
  while(tmp->kids != NULL)
  {
    if(compare_by_addr(&n, &dstwallet.dst))
    {
      macro = TRANSACTION;
      Write(n.fd, &macro, sizeof(macro));
      printf("Sending block to %s:%hu\n", n.address, n.port);
      sendBlock(n.fd, b);
      break;
    }
    tmp = tmp->kids;
    n = *(node_t *)tmp->info;
  }
  printf("Flooding ended\n");
}


/***********************
* STDIN MENU FUNCTIONS *
***********************/
void connect_to_network()
{
  use_node_t new_conn;
  request_t macro = NODE_CONNECTION;
  // try to connect to node_n node
  // TODO: check ip address and retry
  // TODO: check port

  choose_node(&new_conn);

  if(new_conn.confirm == 'y')
  {
    int bsize_new_conn = 0;
    int fd = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in tmp;

    pthread_rwlock_rdlock(&bchain_mtx);
    int currentbchain_size = blockchain->b_size;
    pthread_rwlock_unlock(&bchain_mtx);

    Block b = (Block)Malloc(BLOCK_SIZE);

    fillAddressIPv4(&tmp, new_conn.n.address, new_conn.n.port);
    new_conn.n.fd = fd;

    //visitConnectedNode(&new_conn.n);

    if (connect(fd, (struct sockaddr *)&tmp, sizeof(struct sockaddr)) != -1)
    {
      Write(fd, &macro, sizeof(macro));

      // waiting confirming in response
      printf("\nAspetto un intero che è il size della SUA blochchain\n");
      Read(fd, &bsize_new_conn, sizeof(bsize_new_conn));
      printf("Size received of his blockchain: %d\n", bsize_new_conn);

      printf("Mando un intero che è il size della mia blockchain\n");
      Write(fd, &currentbchain_size, sizeof(currentbchain_size));
      printf("Sent my blockchain size[%d]\n", currentbchain_size);

      /*
      /*************************
       **   SYNC BLOCKCHAIN   **
       ************************ /

      int diff = currentbchain_size - bsize_new_conn;
      int level;
      printf("Diff = his blockchain size [%d] - blockchain size[%d] = %d\n", bsize_new_conn, currentbchain_size, diff);
      
      //if his blockchain is bigger, receive
      if(diff > 0)
      {
        printf("Sending block starting from %d-st block of blockchain\n", currentbchain_size - diff);
        for(level = (currentbchain_size - diff)+1; level <= currentbchain_size; level++)
        {
          printf("Searchin by level\n");

          pthread_rwlock_rdlock(&bchain_mtx);
          b = searchByLevel(blockchain, level);
          pthread_rwlock_unlock(&bchain_mtx);

          printf("Sending block %d\n",level);
          sendBlock(fd, b);

          printf("Sent block %d\n",level);
          visitBlock(b);
        }
      }
      else if(diff < 0)// him got more block than me
      {
        level = 0;
        // ( blockchain_size - diff  = livello da cui partire a mandare )
        // e.g.: blockchain_size = 15, size = 3, level to search in tree is 12
        while(level > diff)
        {
          recvBlock(fd, b);
          visitBlock(b);

          printf("Adding to mine blockchain\n");

          pthread_rwlock_wrlock(&bchain_mtx);
          addBlockToBlockchain(blockchain, b);
          pthread_rwlock_unlock(&bchain_mtx);

          printf("Starting flooding\n");
          //spread_block(b,fd);
          level--;
        }
      }
      */
      printf("*****BLOCKCHAIN SYNCED******\n");
      /*************************
       * downloaded blockchain *
       **  adding to my list  **
       *************************/

      fd_open[fd] = 1;
      if(fd > max_fd)
        max_fd = fd;

      node_t *node = (node_t *)Malloc(sizeof(node_t));
      node->fd = new_conn.n.fd;
      node->port = new_conn.n.port;
      strncpy(node->address, new_conn.n.address, LEN_ADDRESS);

      pthread_rwlock_wrlock(&node_mtx);
      create_kid_to_node(connected_node, node);
      pthread_rwlock_unlock(&node_mtx);

    }
    //failed connect
    else
      perror("connect_to_network");
  }
  else
    printf("Aborting...");
}


void close_connection()
{
  use_node_t node;
  choose_node(&node);
  printf("Searching ->");
  node_t *ptrnode = (node_t *)Malloc(sizeof(node_t));

  ptrnode->fd = node.n.fd;
  ptrnode->port = node.n.port;
  strncpy(ptrnode->address, node.n.address, LEN_ADDRESS);

  visitConnectedNode(ptrnode);

  pthread_rwlock_wrlock(&node_mtx);
  Tree found = remove_from_tree(connected_node, ptrnode, compare_by_addr);
  pthread_rwlock_unlock(&node_mtx);

  if(found != NULL)
  {
    printf("Found connected node with that IP:PORT\n");
    printf("******Closing connection*****\n");
    //ptrnode = (node_t *)found->info;
    //visitConnectedNode(ptrnode);

    fd_open[node.n.fd] = 0;

    close(node.n.fd);
    free(ptrnode);
  }
  else
    printf("Node not found\n");
}


void menu_case(int choice)
{
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
      exit(EXIT_SUCCESS);
    default:
      fprintf(stderr, "Choice is not processed, retry\n");
    break;
  }
}


/**********************
    THREAD FUNCTIONS
***********************/
void* node_connection(void* arg)
{
  // adding node to my custom list after confirming
  int fd = *((int*)arg);
  free(arg);
  node_t n;
  int currentbchain_size = 0;
  int bsize_n = 0;

  Block b = (Block) Malloc(BLOCK_SIZE);

  //sending confirm
  fprintf(stderr, "\nInvio un intero che è il size della mia blockchain\n");

  // blocking or non blocking (?)
  pthread_rwlock_rdlock(&bchain_mtx);
  currentbchain_size = blockchain->b_size;
  pthread_rwlock_unlock(&bchain_mtx);

  Write(fd, &currentbchain_size, sizeof(currentbchain_size));
  fprintf(stderr, "Sent size of my blockchain: [%d]\n", currentbchain_size);


  fprintf(stderr, "Ricevo un intero che è il size della SUA blockchain andando a sovrascriver -> %d\n",bsize_n);
  Read(fd, &bsize_n, sizeof(bsize_n));
  fprintf(stderr, "Received his blockchain size: [%d]\n", bsize_n);

  /*
  int diff = currentbchain_size - bsize_n;
  printf("Diff = his blockchain size [%d] - blockchain size[%d] = %d\n", bsize_n, currentbchain_size, diff);
  
  int level;

  //if my blockchain is bigger send
  if(diff > 0)
  {
    printf("Sending block starting from %d-st block of blockchain\n", currentbchain_size - diff);
    for(level = (currentbchain_size - diff)+1; level <= blockchain->b_size; level++)
    {
      printf("Searchin by level\n");

      pthread_rwlock_rdlock(&bchain_mtx);
      b = searchByLevel(blockchain, level);
      pthread_rwlock_unlock(&bchain_mtx);

      printf("Sending block %d\n", level);
      sendBlock(fd, b);

      printf("Sent block %d\n", level);
    }
  }
  else if(diff < 0) // i must receive
  {
    level = 0;
    printf("Receiving block \n");
    while(level > diff)
    {
      printf("Ricevo blocco\n");
      recvBlock(fd, b);
      visitBlock(b);
      printf("Adding to mine blockchain\n");

      pthread_rwlock_wrlock(&bchain_mtx);
      addBlockToBlockchain(blockchain, b);
      pthread_rwlock_unlock(&bchain_mtx);

      //printf("Starting flooding\n");
      //spread_block(b,fd);
      level--; 
    }
  }
   */
  printf("*****BLOCKCHAIN SYNCED******\n");

  getpeerNode(fd, &n);

  node_t *node = (node_t *)Malloc(sizeof(node_t));
  node->fd = n.fd;
  node->port = n.port;
  strncpy(node->address, n.address, LEN_ADDRESS);

  fd_open[fd] = 1;
  if(fd > max_fd)
    max_fd = fd;

  pthread_rwlock_wrlock(&node_mtx);
  create_kid_to_node(connected_node, node);
  pthread_rwlock_unlock(&node_mtx);

  printf("Exiting from thread that manages connections...\n");
  pthread_exit(NULL);
}


void* wallet_connection(void *arg)
{
  int fd = *((int*)arg);
  free(arg);
  node_t n;
  getpeerNode(fd, &n);
  int confirm = 1;
  Write(fd, &confirm, sizeof(confirm));

  node_t *wallet = (node_t *)Malloc(sizeof(node_t));
  wallet->fd = n.fd;
  wallet->port = n.port;
  strncpy(wallet->address, n.address, LEN_ADDRESS);

  pthread_rwlock_wrlock(&node_mtx);
    create_kid_to_node(connected_wallet, wallet);
  pthread_rwlock_unlock(&node_mtx);


  fd_open[fd] = 1;
  if(fd > max_fd)
    max_fd = fd;

  printf("fd new wallet connection: %d\n",fd);
  pthread_exit(NULL);
}


void* receive_transaction(void *arg)
{
  int fd = *((int*)arg);
  free(arg);
  trns_t trns;
  int confirm = 1;
  // receiv transaction from w_node
  Read(fd, &trns, sizeof(trns));
  printf("package from %s:%d\n", trns.src.address, trns.src.port);
  // send confirm
  Write(fd, &confirm, sizeof(confirm));

  //creating block with transaction
  Block b = create_block(trns);

  //waiting rand sec
  printf("Waiting %d sec \n", b->randomtime);
  //sleep((unsigned int)b->randomtime);

  printf("Block[%d]\n",b->n_block);

  //if created block is with already old(received another block)
  pthread_rwlock_rdlock(&bchain_mtx);
  while(b->n_block <= blockchain->b_size)
  {
    pthread_rwlock_unlock(&bchain_mtx);
    printf("Recreating block\n");
    b = create_block(trns);
  }//recreate

  printf("Adding to blockchain\n");

  pthread_rwlock_wrlock(&bchain_mtx);
  addBlockToBlockchain(blockchain, b);
  pthread_rwlock_unlock(&bchain_mtx);

  //spread_block();
  pthread_exit(NULL);
}


void *receive_block(void *arg)
{
  int fd = *((int*)arg);
  free(arg);
  // received from a thread
  Block b = NULL;

  recvBlock(fd, b);
  printf("Received new block\n");

  pthread_rwlock_wrlock(&bchain_mtx);
  addBlockToBlockchain(blockchain, b);
  pthread_rwlock_unlock(&bchain_mtx);

  //spread to others:
  printf("Start flooding\n");
  spread_block(b, fd);

  pthread_exit(NULL);
}


/******************
   N_NODE ROUTINE
******************/
void n_routine()
{
  srand(time(NULL));

  //sig_action and masks
  /*struct sigaction sig_act;
  sig_act.sa_handler = sig_handler;
  sig_act.sa_flags = 0;

  sigset_t new_mask, old_mask;
  sigemptyset(&sig_act.sa_mask);
  sigaction(SIGINT, &sig_act, NULL);

  sigemptyset(&new_mask);
  sigemptyset(&old_mask);
  sigaddset(&new_mask, SIGINT);
  sigprocmask(SIG_SETMASK, NULL, &old_mask);*/

  //mutex dinamically allocated
  pthread_rwlock_init(&bchain_mtx, NULL);
  pthread_mutex_init(&fd_mtx, NULL);
  pthread_rwlock_init(&node_mtx, NULL);

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

  // creating list for connected node;
  connected_node = new_node(NULL, NULL, NULL);
  // Creating list for connected wallet;
  connected_wallet = new_node(NULL, NULL, NULL);
  // creating blockchain;
  blockchain = create_blockchain();

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
  fd_open = (int *)calloc(FD_SETSIZE, sizeof(int));

  // setting list_fd as max
  fd_open[list_fd] = 1;
  max_fd = list_fd;

  while (1)
  {
    // critic section used to control flag
    //sigprocmask(SIG_BLOCK, &new_mask, NULL);

    //if(exit_flag == 1)
    //  break;

    FD_ZERO(&fdset);
    FD_SET(STDIN_FILENO, &fdset);
    FD_SET(list_fd, &fdset);
    //re update fdset with fd_open monitor table
    for (i_fd = 0; i_fd <= max_fd; i_fd++)
      if(fd_open[i_fd])
        FD_SET(i_fd, &fdset);


    //system("clear");
    printf("\tChoose:\n1) connect to peers;\n2) disconnect from peer\n5) quit\n");
    pthread_rwlock_rdlock(&node_mtx);
    visit_tree(connected_node, visitConnectedNode);
    printf("\n");
    visit_tree(connected_wallet, visitConnectedWallet);
    pthread_rwlock_unlock(&node_mtx);
    printf("\n");
    pthread_rwlock_rdlock(&bchain_mtx);
    visit_tree(blockchain->genesis, visitBlock);
    pthread_rwlock_unlock(&bchain_mtx);

    n_ready = select(max_fd + 1, &fdset, NULL, NULL, NULL); //reset

    if(n_ready < 0 && errno == EINTR)
    {
      //sigprocmask(SIG_UNBLOCK, &new_mask, NULL); // unlock if signal's != SIGINT
      continue;
    }


    if(n_ready < 0)
    {
      perror("select error");
      exit(1);
    }


    //sigprocmask(SIG_UNBLOCK, &new_mask, NULL);
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
      menu_case(choice);
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
        exit(1);
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
        fflush(stdin);
        response = Read(i_fd, &request, sizeof(request));

        /************************************
         * if closed because EOF while Read;
         ************************************/
        if(response != 0)
        {
          fprintf(stderr, "Closed connection\n");
          //closing
          fd_open[i_fd] = 0;
          close(i_fd);

          Tree found = NULL;
          node_t *tmp_node = (node_t *)Malloc(sizeof(node_t));
          tmp_node->fd = i_fd;

          found = remove_from_tree(connected_node, tmp_node, compare_by_fd);
          if(found != NULL)
            free(found);

          found = remove_from_tree(connected_wallet, tmp_node, compare_by_fd);
          if(found != NULL)
            free(found);


          //updating max fd with the last open in fd_open
          if(max_fd == i_fd)
          {
            while (fd_open[--i_fd] == 0)
              ;
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
            fprintf(stderr,"*********Creating thread for node_connection*********\n");
            pthread_create(&tid[tid_index], NULL, node_connection, arg);
            break;
          case WALLET_CONNECTION:
            fprintf(stderr,"********Creating thread for wallet_connection*********\n");
            pthread_create(&tid[tid_index], NULL, wallet_connection, arg);
            break;
          case TRANSACTION:
            fprintf(stderr,"*******Creating thread for receive_transaction********\n");
            pthread_create(&tid[tid_index], NULL, receive_transaction, arg);
            break;
          case BLOCK_SPREAD:
            fprintf(stderr,"*******Creating thread for receive_block (spreading)*******\n");
            pthread_create(&tid[tid_index], NULL, receive_block, arg);
            break;
          default:
            fprintf(stderr, "Node: request received is not correct\n");
            break;
        }
        tid_index++;
      }
    }
  }

  printf("Closing node_handler\n");
  for (i_fd = 0; i_fd <= max_fd; i_fd++)
  {
    if (fd_open[i_fd])
    {
      printf("Closing fd_open[i_fd]: %d", fd_open[i_fd]);
      close(fd_open[i_fd]);
    }
  }

  free(fd_open);

  pthread_rwlock_destroy(&bchain_mtx);
  pthread_mutex_destroy(&fd_mtx);
  pthread_rwlock_destroy(&node_mtx);

  for(int j=0; j < tid_index;  j++)
    pthread_kill(tid[j], SIGKILL);

  exit(EXIT_SUCCESS);
}
