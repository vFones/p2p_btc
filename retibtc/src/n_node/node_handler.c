#include "n_node.h"

static pthread_mutex_t mtx_tree;

struct confirm_new_node{
  Conn_node node;
  char confirm;
};

static struct confirm_new_node choose_node()
{
  //char buffer[256];
  char buffer[BUFFLEN];
  struct confirm_new_node new_node;

  new_node.node = (Conn_node)Malloc(CONN_NODE);

  printf("\nInsert a valid IPv4 address: ");
  scanf(" %s", buffer);
  strncpy(new_node.node->address, buffer, 32);

  printf("Insert a valid port address: ");
  scanf(" %hd", &new_node.node->port);

  printf("Are those info correct? Press [y] to retry, any other char to skip node\n");
  scanf(" %c", &new_node.confirm);
  if(new_node.confirm == 'y')
    return new_node;
  else
  {
    printf("Info not correct\n");
    return new_node;
  }
}

static void connect_to_network()
{
  int node_n = 0, response = 0, i = 0;
  struct confirm_new_node new_conn;

  printf("\nHow many node do you want to connect to?: ");
  fflush(stdin);
  scanf(" %d", &node_n);

  int succ_connection = 0;
  // try to connect to node_n node
  // TODO: check ip address and retry
  // TODO: check port
  while (i < node_n)
  {
    printf("Node [%d]", i);

    new_conn = choose_node();

    if(new_conn.confirm == 'y')
    {
      int fd = Socket(AF_INET, SOCK_STREAM, 0);
      struct sockaddr_in tmp;
      fillAddressIPv4(&tmp, new_conn.node->address, new_conn.node->port);
      Connect(fd, (struct sockaddr *)&tmp);
      sendInt(fd, NODE_CONNECTION);

      recvInt(fd, &response);
      //if response is positive add to my list.
      if(response)
      {
        new_conn.node->fd = fd;
        pthread_mutex_lock(&mtx_tree);
          create_kid_to_node(connected_node, new_conn.node);
        pthread_mutex_unlock(&mtx_tree);
        succ_connection++;
      }
      else
        printf("Node didn't accept, skip and go on\n");
    }
    i++;
  }

  printf("Connected to %d node\n", succ_connection);

  pthread_mutex_lock(&mtx_tree);
    visit_tree(connected_node, visitConnectedNode);
  pthread_mutex_unlock(&mtx_tree);

  fd_open[new_conn.node->fd] = 1;
  if(new_conn.node->fd > max_fd)
    max_fd = new_conn.node->fd;

  return;
}

static void* node_connection(void* arg)
{
  // adding node to my custom list after confirming
  int fd = *(int*)arg;
  Conn_node n = (Conn_node)malloc(CONN_NODE);

  n = getConnectedNode(fd, n);

  sendInt(n->fd, 1);

  pthread_mutex_lock(&mtx_tree);
    create_kid_to_node(connected_node, n);
  pthread_mutex_unlock(&mtx_tree);

  fd_open[n->fd] = 1;
  if(n->fd > max_fd)
    max_fd = n->fd;

  //TODO: download blockchain
  fprintf(stderr,"node_connection: pthread exit\n");
  pthread_exit(NULL);
  return NULL;
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
    break;

    default:
      fprintf(stderr, "Choice is not processed, retry\n");
    break;
  }
  return;
}

void n_routine()
{
  fprintf(stderr, "I'm [%d] forked from [%d]\n", getpid(), getppid());
  int optval = 1;
  int list_fd = 0;
  list_fd = Socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(list_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
  //binding address on list_fd and setting queue

  pthread_mutex_init(&mtx_tree, NULL);

  struct sockaddr_in my_server_addr;
  fillAddressIPv4(&my_server_addr, NULL, node_port);

  Bind(list_fd, (struct sockaddr*)&my_server_addr);
  Listen(list_fd, BACKLOG);

  connected_node = new_node(NULL, NULL, NULL);

  // used for select management and monitor
  int i_fd = 0, n_ready = 0;
  fd_set fdset;
  int fd = 0;

  int response = 0, request = 0;

  // thread stuff
  pthread_t tid[BACKLOG];
  int tid_args[BACKLOG];
  int tid_index = 0;

  // used for switch case menu
  int choice = 0;
  char line_buffer[16];

  //settin max_fd as list_fd and monitoring that on fd_open table
  fd_open = (int *)calloc(FD_SETSIZE, sizeof(int));
  max_fd = list_fd;
  fd_open[max_fd] = 1;

  while (1)
  {
    FD_ZERO(&fdset);
    FD_SET(STDIN_FILENO, &fdset);
    FD_SET(list_fd, &fdset);
    //re update fdset with fd_open monitor table
    for (i_fd = 0; i_fd <= max_fd; i_fd++)
      if (fd_open[i_fd])
        FD_SET(i_fd, &fdset);

    while ( (n_ready = select(max_fd + 1, &fdset, NULL, NULL, NULL)) < 0 );
    if (n_ready < 0 )
    { /* on real error exit */
      perror("select error");
      exit(1);
    }

    /*******************
        STDIN menu_case
    *******************/
    if(FD_ISSET(STDIN_FILENO, &fdset))
    {
      n_ready--;
      printf("\tChoose:\n1) connect to peers;\n2) disconnect from peer\n5) quit\n");
      printf("Connected node:\n");
      visit_tree(connected_node, visitConnectedNode);

      fflush(stdin);
      fgets(line_buffer, 16, stdin);
      choice = atoi(line_buffer);
      menu_case(choice);
    }

    /***************************
        Socket connections
    ***************************/
    if (FD_ISSET(list_fd, &fdset))
    {
      n_ready--; //accepting after new connection
      int keepalive = 1;
      if ((fd = accept(list_fd, NULL, NULL)) < 0)
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
      if (!fd_open[i_fd])
      continue;

      // if socket wakeup
      if (FD_ISSET(i_fd, &fdset))
      {
        n_ready--;
        response = recvInt(i_fd, &request);
        if (response != 0)
        {
          fprintf(stderr,"Closed connection\n");
          //closing and choosing new max fd to monitor
          fd_open[i_fd] = 0;
          close(i_fd);
          Tree found = remove_from_tree(connected_node, (void*)&i_fd, compare_by_fd);
          if(found != NULL)
            free(found);
          // TODO: removing from list if crashed

          //updating max fd with the last open in fd_open
          if (max_fd == i_fd)
          {
            while (fd_open[--i_fd] == 0)
              ;
            max_fd = i_fd;
            break;
          }
          continue;
        }

        tid_args[tid_index%BACKLOG] = i_fd;
        //request received correctly, switching between cases
        switch(request)
        {
          case NODE_CONNECTION:
            pthread_create(&tid[tid_index%BACKLOG], NULL, node_connection, (void *)&tid_args[tid_index%BACKLOG]);
            break;
          /*
          case WALLET_CONNECTION:
          wallet_connection();
          break;
          case RECV_TRNS:
          receive_transaction();
          break;
          */
          default:
            fprintf(stderr, "Wallet[%d]: request received is not correct\n", getpid());
            //print_menu();
            break;
        }
        tid_index++;
      }
    }
    //TODO: move to handler
    for(int j=0; j < tid_index;  j++)
      pthread_join(tid[j], NULL);
  }
}
