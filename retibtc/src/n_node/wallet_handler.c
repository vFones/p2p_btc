#include "n_node.h"
#include "../../include/transaction.h"

static pthread_mutex_t mtx_tree;

static void* wallet_connection(void *arg)
{
  //auth
  int fd = *(int*)arg;
  Conn_node n = NULL;
  n = getpeerNode(fd);

  sendInt(n->fd, 1);

  pthread_mutex_lock(&mtx_tree);
    create_kid_to_node(connected_wallet, n);
  pthread_mutex_unlock(&mtx_tree);

  fprintf(stderr,"wallet_connection: pthread exit\n");

  pthread_exit(NULL);
}

static void* receive_transaction(void *arg)
{
  int fd = *(int*)arg;
  struct transaction trns;
  int response = 0;
  printf("Wallet_handler[%d]: receiving transaction...\n", getpid());
  // receiv transaction from w_node and write down to node_handler
  Read(fd, &trns, sizeof(trns));
  Write(fifo_fd, &trns, sizeof(trns));

  //recvInt(fifo_fd, &response);
  //printf("received from fifo confirm: %d\n", response);

  sendInt(fd, 1);

  pthread_exit(NULL);
}


void w_routine()
{
  printf("I'm [%d] forked from [%d]\n", getpid(), getppid());

  exit_flag = 0;

  pthread_mutex_init(&mtx_tree, NULL);

  int list_fd = 0;
  list_fd = Socket(AF_INET, SOCK_STREAM, 0);

  int opt_value = 1;
  setsockopt(list_fd, SOL_SOCKET, SO_REUSEADDR, &opt_value, sizeof(int));

  //sig_action
  sig_act.sa_handler = sig_handler;
  sig_act.sa_flags = 0;

  sigset_t new_mask, old_mask;
  sigemptyset(&sig_act.sa_mask);
  sigaction(SIGINT, &sig_act, NULL);
  sigaddset(&new_mask, SIGINT);
  sigprocmask(SIG_SETMASK, NULL, &old_mask);

  //opening fifo
  fifo_fd = open(FIFOPATH, O_RDWR);

  struct sockaddr_in my_server_addr;
  fillAddressIPv4(&my_server_addr, NULL, wallet_port);

  //binding address on list_fd and setting queue
  Bind(list_fd, (struct sockaddr*)&my_server_addr);
  Listen(list_fd, BACKLOG);

  // used for select management and monitor
  int i_fd = 0, n_ready = 0;
  fd_set fdset;
  int fd = 0;
  int request = 0, response = 0;

  // thread stuff
  pthread_t tid[BACKLOG];
  int tid_args[BACKLOG];
  int tid_index = 0;

  // Creating list for connected wallet
  connected_wallet = new_node(NULL, NULL, NULL);

  //settin max_fd as list_fd and monitoring that on fd_open table
  fd_open = (int *)calloc(FD_SETSIZE, sizeof(int));
  max_fd = list_fd;
  fd_open[max_fd] = 1;

  while (1)
  {
    // critic section used to control flag
    sigprocmask(SIG_BLOCK, &new_mask, NULL);

    if(exit_flag == 1)
      break;

    FD_ZERO(&fdset);
    FD_SET(list_fd, &fdset);
    //re update fdset with fd_open monitor table
    for (i_fd = 0; i_fd <= max_fd; i_fd++)
      if (fd_open[i_fd])
        FD_SET(i_fd, &fdset);

    visit_tree(connected_wallet, visitConnectedWallet);

    while ( (n_ready = pselect(max_fd + 1, &fdset, NULL, NULL, NULL, &old_mask)) < 0 );

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


    /***************************
        Socket connections
    ***************************/
    if (FD_ISSET(list_fd, &fdset))
    {
      n_ready--; //accepting after new connection
      int keepalive = 1;
      if( (fd = accept(list_fd, NULL, NULL)) < 0 )
      {
        perror("accept");
        exit(1);
      }
      setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));

      // setting fd ready for reading
      fd_open[fd] = 1;

      //calculating new max
      if (max_fd < fd)
        max_fd = fd;
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
          //closing and choosing new max fd to monitor
          fd_open[i_fd] = 0;
          close(i_fd);
          Tree found = remove_from_tree(connected_wallet, (void *) &i_fd, compare_by_fd);
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
        /****************/

        tid_args[tid_index] = i_fd;
        switch (request)
        {
          case WALLET_CONNECTION:
            pthread_create(&tid[tid_index], NULL, wallet_connection, (void *) &tid_args[tid_index]);
            break;
          case TRANSACTION:
            pthread_create(&tid[tid_index], NULL, receive_transaction, (void *) &tid_args[tid_index]);
            break;
          default:
            fprintf(stderr, "Wallet[%d]: request received is not correct\n", getpid());
            break;
        }
        tid_index++;
      }
    }
  }
  //free(connected_wallet);
  for (i_fd = 0; i_fd <= max_fd; i_fd++)
    if (fd_open[i_fd])
      close(fd_open[i_fd]);

  //for(int j=0; j < tid_index;  j++)
  //  pthread_join(tid[j], NULL);

  pthread_mutex_destroy(&mtx_tree);

  exit(EXIT_SUCCESS);
}
