#include "n_node.h"
#include "../../include/blockchain.h"


void* request_handler(void *arg);

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
    fprintf(stderr,"Block [%d] <---> ", b->n_block);
    visitTransaction(trns);
  }
}


Block create_block(Trns trns)
{
  Block b = (Block)Malloc(BLOCK_SIZE);

  b->info = trns;

  pthread_rwlock_rdlock(&bchain_mtx);
  Block latest = getBlockFromNode(blockchain->tail);
  pthread_rwlock_unlock(&bchain_mtx);

  b->n_block = latest->n_block + 1;
  b->randomtime = 5+(rand()%11);
  strncpy(b->creator.address, "0.0.0.0", LEN_ADDRESS);
  b->creator.port = node_server.port;

  fprintf(stderr,"Created block\n");

  return b;
}


int spread_block(Block b, int fd, bool first)
{
  //send to everyone exept the one who sended to me (control via fd)
  node_t n;
  getpeerNode(fd, &n);
  request_t macro;

  Trns t = b->info;

  char mypublicip[LEN_ADDRESS];
  strncpy(mypublicip, "0.0.0.0", LEN_ADDRESS);

  // before start flooding i check if i created the block with flag first
  // if i received the block stop propagation if i'm the creator
  if(!first)
  {
    if(!(strncmp(b->creator.address, mypublicip, LEN_ADDRESS)) && (b->creator.port == node_server.port))
    {
      fprintf(stderr, "I created the block, no need to resend\n");
      return 1;
    }
  }

  // WALLET PROPAGATION

  fprintf(stderr, "Wallet propagation\n");
  macro = TRANSACTION;
  /**send to the wallet if connected to me**/
  pthread_rwlock_rdlock(&node_mtx);
  for(int i = 0; i <= wallet_list_size; i++)
  {
    // if wallet exist ( != 0 ) in my table
    if( (wallet_list[i].fd != 0 && wallet_list[i].port != 0) )
    {
      //if wallet is different from who sended to me
      if( wallet_list[i].port != n.port && !(strncmp(wallet_list[i].address, n.address, LEN_ADDRESS)) )
      {
        if( wallet_list[i].port == t->dst.port  && (strncmp(wallet_list[i].address, t->dst.address, LEN_ADDRESS) == 0) ) // different from destinatary
        {
          Write(wallet_list[i].fd, &macro, sizeof(macro));
          //waiting confirm;
          Write(wallet_list[i].fd, t, TRNS_SIZE);
        }
      }
    }
  }
  pthread_rwlock_unlock(&node_mtx);

  fprintf(stderr, "NODE PROPAGATION\n");
  macro  = BLOCK_SPREAD;
  //scroll list and sent to everyone

  pthread_rwlock_rdlock(&node_mtx);
  for(int i = 0; i <= node_list_size; i++)
  {
    if(node_list[i].fd != 0 && node_list[i].port != 0)
    {
      if(node_list[i].port != n.port && !(strncmp(node_list[i].address, n.address, LEN_ADDRESS)))
      {
        fprintf(stderr, "Sending to NODE [%d][%d]", node_list[i].fd, node_list[i].port);
        Write(node_list[i].fd, &macro, sizeof(macro));
        //waiting confirm
        sendBlock(node_list[i].fd, b);
      }
    }
  }
  pthread_rwlock_unlock(&node_mtx);

  fprintf(stderr,"Flooding ended\n");
  return 0;
}


/***********************
* STDIN MENU FUNCTIONS *
***********************/
int connect_to_network()
{
  use_node_t new_conn;
  request_t macro = NODE_CONNECTION;
  int canconnect = 0;
  int fd = 0;

  choose_node(&new_conn);


  if( node_server.port == new_conn.n.port &&
      ((strncmp(new_conn.n.address, node_server.address, LEN_ADDRESS) == 0) ||
       (strncmp(new_conn.n.address, "127.0.0.1", LEN_ADDRESS)) == 0) )
  {
    fprintf(stderr, "------ Trying to going in a deadlock. Be careful. ------\n");
    return 0;
  }

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
    return 0;
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
      int level = 0;

      if(diff > 0) // difference positive, i got more blocks
      {
        int starting = currentbchain_size - diff;
        fprintf(stderr, "Sending block(s) %d\n", diff);
        for(level = (starting)+1; level <= diff; level++)
        {
          Block b = NULL;

          pthread_rwlock_rdlock(&bchain_mtx);
          b = searchByLevel(blockchain, level);
          pthread_rwlock_unlock(&bchain_mtx);

          visitBlock(b);
          sendBlock(fd, b);
        }
      }
      else if(diff < 0)// he got more block than me
      {
        // ( blockchain_size - diff  = livello da cui partire a mandare )
        // e.g.: blockchain_size = 15, size = 3, level to search in tree is 12
        while(diff)
        {
          Block b = (Block)Malloc(BLOCK_SIZE);
          recvBlock(fd, b);
          visitBlock(b);

          pthread_rwlock_wrlock(&bchain_mtx);
          addBlockToBlockchain(blockchain, b);
          pthread_rwlock_unlock(&bchain_mtx);

          diff++;
          free(b);
          fprintf(stderr, "flooding block\n");
          spread_block(b, fd, false);
        }
      }
      else // DIFF = 0, must check integrity of blockchain
      {
        if(blockchain->b_size > 0)
        {
          int response = 0;
          Block b = getBlockFromNode(blockchain->tail);
          sendBlock(fd, b);

          Read(fd, &response, sizeof(response));
          if(response)
          {
            Block new = (Block)Malloc(BLOCK_SIZE);
            recvBlock(fd, new);

            pthread_rwlock_wrlock(&bchain_mtx);
            addBlockToBlockchain(blockchain, new);
            pthread_rwlock_unlock(&bchain_mtx);
          }
          else
            fprintf(stderr, "Fine sync\n");
        }
      }
      fprintf(stderr, "***** BLOCKCHAIN SYNCED ******\n");
    }
    //failed connect
    else
    {
      perror("connect_to_network");
      return 0;
    }
  }
  else
  {
    fprintf(stderr, "Aborting...\n");
    return 0;
  }

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
  return fd;
}


void close_connection()
{
  use_node_t node;
  choose_node(&node);
  int found = 0;
  int tmp = 0;

  if(node.confirm == 'y')
  {
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
        node_list_size--;
        break;
      }
    }
    pthread_rwlock_unlock(&node_mtx);

    if(found == 1)
    {
      fprintf(stderr, "Found connected node with that IP:PORT\n");
      fprintf(stderr, "******Closing connection*****\n");
      close(tmp);
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
    close(fd);
    fprintf(stderr, "***** got too many peers... quitting connection ******\n");
    return;
  }
  //sending confirm

  pthread_rwlock_rdlock(&bchain_mtx);
  currentbchain_size = blockchain->b_size;
  pthread_rwlock_unlock(&bchain_mtx);

  Write(fd, &currentbchain_size, sizeof(currentbchain_size));
  Read(fd, &bsize_n, sizeof(bsize_n));

  int diff = currentbchain_size - bsize_n;

  int level = 0;

  //if my blockchain is bigger send
  if(diff > 0)
  {
    int starting = currentbchain_size - diff;

    fprintf(stderr, "Sending %d block(s)\n", diff);
    for(level = starting+1; level <= diff; level++)
    {
      Block b = NULL;

      pthread_rwlock_rdlock(&bchain_mtx);
      b = searchByLevel(blockchain, level);
      pthread_rwlock_unlock(&bchain_mtx);

      visitBlock(b);

      visitBlock(b);
      sendBlock(fd, b);
      fprintf(stderr, "Sent block %d\n", level);
    }
  }
  else if(diff < 0) // i must receive
  {
    fprintf(stderr, "Receiving block(s) \n");
    while(diff)
    {
      Block b = (Block)Malloc(BLOCK_SIZE);
      recvBlock(fd, b);
      visitBlock(b);

      pthread_rwlock_wrlock(&bchain_mtx);
      addBlockToBlockchain(blockchain, b);
      pthread_rwlock_unlock(&bchain_mtx);

      diff++;
      free(b);
      printf("Starting flooding\n");
      spread_block(b, fd, false);
    }
  }
  else // DIFF = 0, must check integrity of blockchain
  {
    if(blockchain->b_size > 0)
    {
      int response = 0;
      fprintf(stderr, "Got same blockchain number, recv his block and check\n");
      Block last = getBlockFromNode(blockchain->tail);

      Block new = (Block)Malloc(BLOCK_SIZE);
      recvBlock(fd, new);

      if (compareBlockByInfo(new, last))
      {
        fprintf(stderr, "Got same latest block. Sending 0 as response\n");
        Write(fd, &response, sizeof(response));
      }
      else
      {
        fprintf(stderr, "Latest block is different. Sending 1 as response\n");
        response = 1;
        Write(fd, &response, sizeof(response));

        fprintf(stderr, "Sending block.");
        sendBlock(fd, last);

        pthread_rwlock_wrlock(&bchain_mtx);
        addBlockToBlockchain(blockchain, new);
        pthread_rwlock_unlock(&bchain_mtx);
      }
    }
  }

  fprintf(stderr, "***** BLOCKCHAIN SYNCED ******\n");

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
}


void wallet_connection(void *arg)
{
  int fd = *((int *) arg);
  fprintf(stderr, "********* getting fd new wallet connection \n *********");

  int canconnect = 0;
  node_t n;
  getpeerNode(fd, &n);


  pthread_rwlock_rdlock(&node_mtx);
  for (int i = 0; i <= wallet_list_size; i++)
  {
    if(wallet_list[i].fd == 0 && wallet_list[i].port == 0) // if there is space: can connect
    {
      canconnect = 1;
      break;
    }
    else if(wallet_list[i].port == n.port && !(strncmp(wallet_list[i].address, n.address, LEN_ADDRESS)))
      break; // if there is no space exit
  }
  pthread_rwlock_unlock(&node_mtx);

  if(!canconnect)
  {
    close(fd);
    fprintf(stderr, "***** Cannot connect right now... quitting connection ******\n");
    return;
  }

  int confirm = 1;
  Write(fd, &confirm, sizeof(confirm));

  pthread_rwlock_wrlock(&node_mtx);
  for (int i = 0; i <= wallet_list_size; i++)
  {
    if(wallet_list[i].port == n.port && !(strncmp(wallet_list[i].address, n.address, LEN_ADDRESS)))
    {
      fprintf(stderr, "Already got that wallet in my table.\n");
      pthread_exit(NULL);
    }
    if(wallet_list[i].fd == 0 && wallet_list[i].port == 0)
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
}


void receive_transaction(void *arg)
{
  int fd = *((int*)arg);

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
  sleep((unsigned int)b->randomtime);

  fprintf(stderr, "Block[%d]\n",b->n_block);

  pthread_rwlock_rdlock(&bchain_mtx);
  Block new = getBlockFromNode(blockchain->tail);
  pthread_rwlock_unlock(&bchain_mtx);

  //if created block is with already old ( aka received another block during waiting)
  if(b->n_block < new->n_block)
  {
    fprintf(stderr,"Recreating block\n");
    b = create_block(trns);
    pthread_rwlock_wrlock(&bchain_mtx);
    addBlockToBlockchain(blockchain, new);
    pthread_rwlock_unlock(&bchain_mtx);

    fprintf(stderr,"Adding to blockchain\n");
  }//recreate
  else
  {
    fprintf(stderr,"Adding to blockchain\n");
    pthread_rwlock_wrlock(&bchain_mtx);
    addBlockToBlockchain(blockchain, b);
    pthread_rwlock_unlock(&bchain_mtx);
  }

  fprintf(stderr,"Added to blockchain\n");

  bool first = true;
  spread_block(b, fd, first);

  fprintf(stderr, "**** received transaction ****");
}


void receive_block(void *arg)
{
  int fd = *((int*)arg);
  Block b = (Block)Malloc(BLOCK_SIZE);

  pthread_rwlock_rdlock(&bchain_mtx);
  Block latest_in_bchain = blockchain->tail->info;
  int tmp_size = blockchain->b_size;
  pthread_rwlock_unlock(&bchain_mtx);

  recvBlock(fd, b);
  fprintf(stderr,"Received new block\n");

  //if block progressivity is sequentially next to blockchain and different from latest ADD it
  if(compareBlockByInfo(b, latest_in_bchain))
    fprintf(stderr, "Already in my blockchain, why spreading?\n");


  if( ((b->n_block == tmp_size) || (b->n_block == tmp_size+1)) )
  {
    pthread_rwlock_wrlock(&bchain_mtx);
    addBlockToBlockchain(blockchain, b);
    pthread_rwlock_unlock(&bchain_mtx);

    fprintf(stderr,"Start flooding\n");
    spread_block(b, fd, false);
  }
  else
  {
    fprintf(stderr, "I'm missing too many block [%d] [%d], just spreading\n", b->n_block, blockchain->b_size);
    spread_block(b, fd, false);
  }
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

  fillAddressIPv4(&my_server_addr, NULL, node_server.port);

  // bindin address:port to socket fd and setting a backlog
  Bind(list_fd, (struct sockaddr *) &my_server_addr);
  Listen(list_fd, BACKLOG);
  inet_ntop(AF_INET, &my_server_addr.sin_addr, node_server.address, sizeof(node_server.address));

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

  node_list = (node_t *) calloc(BACKLOG, sizeof(node_t));
  wallet_list = (node_t *) calloc(BACKLOG, sizeof(node_t));

  // creating blockchain;

  // used for select management and monitor
  int n_ready = 0;
  fd_set fdset; //fdset to fill in select while
  int accept_fd = 0;
  int return_fd = 0;

  // thread stuff
  pthread_t *tid = (pthread_t *) Malloc(BACKLOG);
  int tid_index = 0;
  pthread_mutex_init(&t_index_mtx, NULL);
  pthread_attr_t t_detached;
  pthread_attr_init(&t_detached);
  pthread_attr_setdetachstate(&t_detached, PTHREAD_CREATE_DETACHED);

  // used for switch case men\nu
  int choice = 0;
  char line_buffer[16];

  //settin max_fd as list_fd and monitoring that on fd_open table
  pthread_rwlock_init(&fd_mtx, NULL);

  // setting list_fd as max
  max_fd = list_fd;

  while (1)
  {
    // critic section used to control flag
    sigprocmask(SIG_BLOCK, &new_mask, NULL);

    if(exit_flag)
      break;

    FD_ZERO(&fdset);
    FD_SET(STDIN_FILENO, &fdset);
    FD_SET(list_fd, &fdset);

    system("clear");
    fprintf(stderr, "\n* * * * * Connected N_NODE * * * * *\n");

    pthread_rwlock_rdlock(&node_mtx);
    for (int i = 0; i < node_list_size; i++)
      visit_node_list(node_list[i]);
    pthread_rwlock_unlock(&node_mtx);

    fprintf(stderr, "\n");
    fprintf(stderr, "* * * * * Connected W_NODE * * * * *\n");

    pthread_rwlock_rdlock(&node_mtx);
    for (int i = 0; i < wallet_list_size; i++)
      visit_wallet_list(wallet_list[i]);
    pthread_rwlock_unlock(&node_mtx);

    fprintf(stderr, "\n");
    fprintf(stderr, "* * * * * BLOCKCHAIN * * * * *\n");

    pthread_rwlock_rdlock(&bchain_mtx);
    visit_tree(blockchain->genesis, visitBlock);
    pthread_rwlock_unlock(&bchain_mtx);

    fprintf(stderr, "\n");
    fprintf(stderr, "Node %s:[%d], choose:\n1) connect to peers;\n2) disconnect from peer\n5) quit\n", \
      node_server.address, node_server.port);
    fprintf(stderr, ">_ ");

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

    /*********************************
             STDIN menu_case
    *********************************/
    if(FD_ISSET(STDIN_FILENO, &fdset))
    {
      n_ready--;
      fgets(line_buffer, 16, stdin);
      choice = atoi(line_buffer);
      switch (choice)
      {
        case 1:
          return_fd = connect_to_network();
          break;
        case 2:
          close_connection();
          break;
        case 5:
          printf("Cleaning and exiting\n");
          exit_flag = 1;
          break;
        default:
          fprintf(stderr, "Choice is not processed: wrong input\n");
          break;
      }
    }

    /********************************
            Socket connections
    *********************************/
    if(FD_ISSET(list_fd, &fdset))
    {
      n_ready--;
      int keepalive = 1;
      accept_fd = Accept(list_fd, NULL);
      setsockopt(accept_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));

      int *arg = (int *) Malloc(sizeof(int));
      if(tid_index < BACKLOG)
      {
        *arg = accept_fd;
        fprintf(stderr, "\nNUOVA CONNESSIONE NUOVO THREAD\n");
        pthread_create(&tid[tid_index], &t_detached, request_handler, arg);

        pthread_mutex_lock(&t_index_mtx);
        tid_index++;
        pthread_mutex_unlock(&t_index_mtx);
      }
      else
      {
        fprintf(stderr, "ERROR: Too many connections\n");
        close(accept_fd);
      }
    }

    if(return_fd)
    {
      int *arg = (int *) Malloc(sizeof(int));
      if(tid_index < BACKLOG)
      {
        *arg = return_fd;
        fprintf(stderr, "\npasso connessione al thread\n");
        pthread_create(&tid[tid_index], &t_detached, request_handler, arg);

        return_fd = 0;

        pthread_mutex_lock(&t_index_mtx);
        tid_index++;
        pthread_mutex_unlock(&t_index_mtx);
      }
      else
        close(return_fd);
    }
  }
  //not waiting threads so I can exit with C-c and kill everybody

  for (int j = 0; j < tid_index; j++)
    pthread_kill(tid[j], SIGKILL);

  //free (blockchain);

  free(node_list);
  free(wallet_list);

  pthread_rwlock_destroy(&fd_mtx);
  pthread_rwlock_destroy(&bchain_mtx);
  pthread_rwlockattr_destroy(&bchain_mtx_attr);
  pthread_rwlock_destroy(&node_mtx);
  pthread_rwlockattr_destroy(&node_mtx_attr);
  pthread_rwlock_destroy(&closed_flag);
  pthread_rwlockattr_destroy(&closed_flag_attr);

  pthread_mutex_destroy(&t_index_mtx);
  pthread_attr_destroy(&t_detached);


  exit(EXIT_SUCCESS);
}

void* request_handler(void *arg)
{
  int fd = *(int *)arg;
  request_t request = 0;

  fd_set fset;
  FD_ZERO(&fset);

  int maxfd = fd+1;
  int nready = 0;

  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 1000000;

  while(1)
  {
    FD_ZERO(&fset);
    FD_SET(fd, &fset);

    nready = select(maxfd, &fset, NULL, NULL, &timeout);

    if(nready < 0 && errno == EINTR)
    {
      perror("select");
      pthread_exit(NULL);
    }

    if(FD_ISSET(fd, &fset))
    {
      ssize_t response = Read(fd, &request, sizeof(request));
      /************************************
      * if closed because EOF while Read;
      ************************************/
      if(response != 0)
      {
        fprintf(stderr, "******* Peer has closed connection ******\n");

        pthread_rwlock_wrlock(&node_mtx);
        /******* REMOVING NODE *********/
        for (int i = 0; i < node_list_size; i++)
        {
          if(node_list[i].fd == fd)
          {
            node_list[i].fd = 0;
            node_list[i].port = 0;
            memset(&node_list[i].address, 0, sizeof(node_list[i].address));
            node_list_size--;
            break;
          }
        }
        pthread_rwlock_unlock(&node_mtx);

        pthread_rwlock_wrlock(&node_mtx);
        /******* REMOVING WALLET *********/
        for (int i = 0; i < wallet_list_size; i++)
        {
          if(wallet_list[i].fd == fd)
          {
            wallet_list[i].fd = 0;
            wallet_list[i].port = 0;
            memset(&wallet_list[i].address, 0, sizeof(wallet_list[i].address));
            wallet_list_size--;
            break;
          }
        }
        pthread_rwlock_unlock(&node_mtx);
        close(fd);

        pthread_mutex_lock(&t_index_mtx);
        tid_index--;
        pthread_mutex_unlock(&t_index_mtx);

        pthread_exit(NULL);
      }

      switch (request)
      {
        case NODE_CONNECTION:
          fprintf(stderr, "********* Creating thread for node_connection *********\n");
          node_connection(arg);
          break;

        case WALLET_CONNECTION:
          fprintf(stderr, "******** Creating thread for wallet_connection *********\n");
          wallet_connection(arg);
          break;

        case TRANSACTION:
          fprintf(stderr, "******* Creating thread for receive_transaction ********\n");
          receive_transaction(arg);
          break;

        case BLOCK_SPREAD:
          fprintf(stderr, "******* Creating thread for receive_block (spreading) *******\n");
          receive_block(arg);
          break;

        default:
          fprintf(stderr, "Node: request received is not correct\n");
          break;
      }
    }
  }
}
