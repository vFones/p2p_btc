#include "n_node.h"
#include "../../include/blockchain.h"

Blockchain blockchain;

static pthread_mutex_t mtx_tree;

/***********************
  STDIN MENU FUNCTIONS
************************/
static void connect_to_network()
{
  struct confirm_new_node new_conn;

  // try to connect to node_n node
  // TODO: check ip address and retry
  // TODO: check port

  new_conn = choose_node();

  if(new_conn.confirm == 'y')
  {
    int bsize_new_conn = 0;
    int fd = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in tmp;

    fillAddressIPv4(&tmp, new_conn.node->address ,new_conn.node->port);
    visitConnectedNode(new_conn.node);

    if (connect(fd, (struct sockaddr *)&tmp, sizeof(struct sockaddr)) != -1)
    {
      sendInt(fd, NODE_CONNECTION);

      // waiting confirming in response
      recvInt(fd, &bsize_new_conn);

      /*************************
       **   SYNC BLOCKCHAIN   **
       ************************/
      //if response is positive add to my list and sync blockchain
      if(bsize_new_conn)
      {
        int diff = bsize_new_conn - blockchain->b_size;

        // if him got more block than me, send to me
        if(bsize_new_conn >= blockchain->b_size)
        {
          //receiving last block just in case size is the same because multi-head
          sendInt(fd, diff);

          while(diff != 0)
          {
            Block b = (Block)Malloc(BLOCK_SIZE);
            recvBlock(fd, b);
            addBlockToBlockchain(blockchain, b);
            diff--;
          }
        }
        else //i've got more block than him, sending to him
        {
          sendInt(fd, diff);
          diff = abs(diff);
          int level = (blockchain->b_size - diff);
          // ( blockchain_size - diff  = livello da cui partire a mandare )
          // e.g.: blockchain_size = 15, size = 3, level to search in tree is 12
          while(level < blockchain->b_size)
          {
            //search by level in tree
            Block b = searchByLevel(blockchain, level);
            sendBlock(fd, b);
            //struct blockchain block = search_in_tree(blockchain, level)
            //send block
            level++;
          }
        }

        /*************************
         * downloaded blockchain *
         **  adding to my list  **
         ************************/
      }
      //response negative
      else
        printf("Node got 0 block\n");

      fd_open[fd] = 1;
      if(fd > max_fd)
        max_fd = fd;

      new_conn.node->fd = fd;

      pthread_mutex_lock(&mtx_tree);
      create_kid_to_node(connected_node, new_conn.node);
      pthread_mutex_unlock(&mtx_tree);

    }
    //failed connect
    else
      perror("connect_to_network");
  }
  else
    printf("Aborting...");
}


static void close_connection()
{
  struct confirm_new_node node = choose_node();
  printf("Searching ->");
  visitConnectedNode(node.node);

  Tree found = remove_from_tree(connected_node, (void*)node.node, compare_by_addr);

  if(found != NULL)
  {
    printf("Found connected node with that IP:PORT\n");
    printf("******Closing connection*****\n");
    node.node = found->info;
    visitConnectedNode(node.node);
    fd_open[node.node->fd] = 0;
    close(node.node->fd);
  }
  else
    printf("Node not found\n");
  free(node.node);
}


static void menu_case(int choice)
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
static void* node_connection(void* arg)
{
  // adding node to my custom list after confirming
  int fd = *(int*)arg;
  Conn_node n = NULL;

  int bsize_n = 0;

  n = getpeerNode(fd);

  //sending confirm
  sendInt(n->fd, blockchain->b_size);

  //if my blockchain size is 0 don't sync
  if(blockchain->b_size)
  {
    recvInt(n->fd, &bsize_n);

    if (bsize_n >= 0) {
      int level = (blockchain->b_size - bsize_n);
      // ( blockchain_size - bsize_n  = livello da cui partire a mandare )
      // e.g.: blockchain_size = 15, bsize_n = 3, level to search in tree is 12
      while (level < blockchain->b_size) {
        //search by level in tree
        Block b = searchByLevel(blockchain, level);
        sendBlock(fd, b);
        level++;
      }
    } else //received negative number
    {
      int diff = abs(bsize_n);
      while (diff != 0) {
        Block b = (Block)Malloc(BLOCK_SIZE);
        recvBlock(n->fd, b);
        addBlockToBlockchain(blockchain, b);
        diff--;
      }
      //send last block since his got same size as me
    }
  }

  pthread_mutex_lock(&mtx_tree);
    create_kid_to_node(connected_node, n);
  pthread_mutex_unlock(&mtx_tree);

  fd_open[fd] = 1;
  if(fd > max_fd)
    max_fd = fd;

  printf("Connected with ->\t");
  visitConnectedNode(n);

  pthread_exit(NULL);
}


void visitBlock(void *arg)
{
  if(arg == NULL)
    return;
  Block b = (Block)arg;
  if(!b->n_block)
    printf("Block[GENESIS]\n");
  else
  {
    Trns trns = b->info;
    printf("block[%d]\t [%s:%hu -> %s:%hu] [%0.2f BTC] [%d] \n", \
      b->n_block, trns->src.address, trns->src.port, \
      trns->dst.address, trns->dst.port, trns->amount, trns->random);
  }
}


static Block create_block(Trns trns)
{
  Block b = (Block)Malloc(BLOCK_SIZE);
  b->info = trns;
  b->n_block = blockchain->b_size+1;
  b->randomtime = 5+(rand()%11);

  b->prev_SHA256 = getLatestSHA256(blockchain);

  // calculate hash of new block
  unsigned char *tmpb_infoSHA256 = (unsigned char *)Malloc(SHA256_DIGEST_LENGTH);
  SHA256(b->info, sizeof(b->info), tmpb_infoSHA256);

  //create a string with previous hash + new tmp hash
  char *tmpSHA256 = (char*)Malloc(SHA256_DIGEST_LENGTH * 2 + 2);

  strncpy(tmpSHA256, b->prev_SHA256, SHA256_DIGEST_LENGTH);
  strncpy(&tmpSHA256[SHA256_DIGEST_LENGTH], (char *)tmpb_infoSHA256, SHA256_DIGEST_LENGTH);
  tmpSHA256[SHA256_DIGEST_LENGTH*2 + 1] = '\0';
  free(tmpb_infoSHA256);

  // calculate hash of new string just created and assign to block
  unsigned char *hash = (unsigned char *)Malloc(SHA256_DIGEST_LENGTH);
  SHA256((unsigned char *)tmpSHA256, strlen(tmpSHA256), hash);
  free(tmpSHA256);

  b->SHA256 = (char *)Malloc(SHA256_DIGEST_LENGTH*2);

  for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    sprintf(&b->SHA256[i*2], "%02x", hash[i]);
  printf("\n");

  printf("Created block\n");

  return b;
}


static void* wallet_connection(void *arg)
{
  //auth
  int fd = *(int*)arg;
  Conn_node n = NULL;
  n = getpeerNode(fd);

  sendInt(fd, 1);

  pthread_mutex_lock(&mtx_tree);
    create_kid_to_node(connected_wallet, n);
  pthread_mutex_unlock(&mtx_tree);

  fd_open[fd] = 1;
  if(fd > max_fd)
    max_fd = fd;

  printf("fd new wallet connection: %d\n",fd);
  pthread_exit(NULL);
}


static void* receive_transaction(void *arg)
{
  int fd = *(int*)arg;
  Trns trns = (Trns)Malloc(TRNS_SIZE);

  // receiv transaction from w_node

  recvTrns(fd, trns);

  sendInt(fd, 1);

  printf("package from %s:%hu\n", trns->src.address, trns->src.port);

  //creating block with transaction
  Block b = create_block(trns);

  //waiting rand sec
  printf("Waiting %d sec \n", b->randomtime);
  printf("prev_sha: %s\n",b->prev_SHA256);
  printf("sha: %s\n", b->SHA256);
  sleep((unsigned int)b->randomtime);

  printf("Block[%d]\n",b->n_block);
  //for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    //printf("%02x", b.prev_SHA256[i]);


  //if created block is with already old(received another block)
  while(b->n_block <= blockchain->b_size)
  {
    printf("Recreating block\n");
    free(b);
    b = create_block(trns);
  }
    //recreate

  addBlockToBlockchain(blockchain, b);
  printf("Added to blockchain\n");

  //visitBlock(b);

  pthread_exit(NULL);
}


/******************
   N_NODE ROUTINE
******************/
void n_routine()
{
  srand(time(NULL));

  //sig_action
  sig_act.sa_handler = sig_handler;
  sig_act.sa_flags = 0;

  sigset_t new_mask, old_mask;
  sigemptyset(&sig_act.sa_mask);
  sigaction(SIGINT, &sig_act, NULL);

  sigemptyset(&new_mask);
  sigemptyset(&old_mask);
  sigaddset(&new_mask, SIGINT);
  sigprocmask(SIG_SETMASK, NULL, &old_mask);

  //mutex dinamically allocated
  pthread_mutex_init(&mtx_tree, NULL);


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

  int response = 0, request = 0;

  // thread stuff
  pthread_t *tid = (pthread_t *)Malloc(BACKLOG);
  int *tid_args[BACKLOG];
  int tid_index = 0;

  // used for switch case menu
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


    printf("\tChoose:\n1) connect to peers;\n2) disconnect from peer\n5) quit\n");
    visit_tree(connected_node, visitConnectedNode);
    printf("\n");
    visit_tree(connected_wallet, visitConnectedWallet);
    visit_tree(blockchain->genesis, visitBlock);

    while ((n_ready = pselect(max_fd + 1, &fdset, NULL, NULL, NULL, &old_mask)) < 0); //reset

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

    if(exit_flag == 1)
      break;

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
        response = recvInt(i_fd, &request);

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

          found = remove_from_tree(connected_node, (void *) &i_fd, compare_by_fd);
          if(found != NULL)
            free(found);

          found = remove_from_tree(connected_wallet, (void *) &i_fd, compare_by_fd);
          if(found != NULL)
            free(found);


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

        tid_args[tid_index] = (int *)Malloc(sizeof(int));
        *tid_args[tid_index] = i_fd;
        //request received correctly, switching between cases
        switch (request)
        {
          case NODE_CONNECTION:
            pthread_create(&tid[tid_index], NULL, node_connection, (void *)tid_args[tid_index]);
            break;
          case WALLET_CONNECTION:
            pthread_create(&tid[tid_index], NULL, wallet_connection, (void *)tid_args[tid_index]);
            break;
          case TRANSACTION:
            pthread_create(&tid[tid_index], NULL, receive_transaction, (void *)tid_args[tid_index]);
            break;
            /*
            case RECV_TRNS:
              receive_transaction();
              break;
            */
          default:
            fprintf(stderr, "Node: request received is not correct\n");
            //print_menu();
            break;
        }
        tid_index++;
      }
    }
  }

  //destroy from here free(blockchain->genesis);
  //free(connected_node);
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

  //for(int j=0; j < tid_index;  j++)
  //  pthread_join(tid[j], NULL);

  pthread_mutex_destroy(&mtx_tree);

  exit(EXIT_SUCCESS);
}

void sig_handler(int sig_no)
{
  if(sig_no == SIGINT)
  {
    printf("\nCaptured C-c, closing [%d].\n", getpid());
    exit_flag = 1;
  }
}
