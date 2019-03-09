#include "n_node.h"


static void connect_to_network()
{
  char addr[LEN_ADDRESS];
  short port = 0;
  char c, buffer[256];
  int node_n = 0, response = 0, i = 0;
  Conn_node to_conn = NULL;

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

    printf("\nInsert a valid IPv4 address: ");
    scanf(" %s", buffer);
    strncpy(addr, buffer, 32);

    printf("Insert a valid port address: ");
    scanf(" %hd", &port);

    printf("Are those info correct? Press [y] to retry, any other char to skip node\n");
    scanf(" %c", &c);

    if(c == 'y')
    {
      int fd = Socket(AF_INET, SOCK_STREAM, 0);
      if ( (to_conn = (Conn_node)malloc(CONN_NODE) ) == NULL)
        perror("malloc");

      fillAddressIPv4(&to_conn->node_addr, addr, port);
      Connect(fd, (struct sockaddr *)&to_conn->node_addr);

      sendInt(fd, NODE_CONNECTION);

      visitConnectedNode(to_conn);

      recvInt(fd, &response);
      //if response is positive add to my list.
      if(response)
      {
        printf("Connected to peer\n");
        to_conn->fd = fd;
        add_kid(connected_node, to_conn);
        //add_to_list(connected_node, to_conn);
        succ_connection++;
      }
      else
        printf("Node didn't accept, skip and go on");
    }
    i++;
  }

  printf("Connected to %d node\n", succ_connection);
  visit_tree(connected_node, visitConnectedNode);

  //visit_list(connected_node, visitConnectedNode);

  pthread_mutex_lock(&mutex);
    fd_open[to_conn->fd] = 1;
    if(to_conn->fd > max_fd)
      max_fd = to_conn->fd;
  pthread_mutex_unlock(&mutex);

  return;
}


static void* node_connection(void* arg)
{
  // adding node to my custom list after confirming
  int fd = *(int*)arg;
  Conn_node n = (Conn_node)malloc(CONN_NODE);

  n = getConnectedNode(fd, n);
  visitConnectedNode(n);

  // probably not necessary search_by_info
  sendInt(n->fd, 1);
  pthread_mutex_lock(&mutex);
    fd_open[n->fd] = 1;
    if(n->fd > max_fd)
      max_fd = n->fd;
    add_kid(connected_node, n); //TEST: add_kid
  pthread_mutex_unlock(&mutex);

  //download blockchain
  pthread_exit(NULL);
}

static void menu_case(int choice)
{
  switch(choice)
  {
    case 1:
    connect_to_network();
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

  struct sockaddr_in my_server_addr;
  fillAddressIPv4(&my_server_addr, NULL, node_port);

  Bind(list_fd, (struct sockaddr*)&my_server_addr);
  Listen(list_fd, BACKLOG);

  connected_node = new_node(NULL);

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

  fd_open = (int *)calloc(FD_SETSIZE, sizeof(int));

  max_fd = list_fd;
  fd_open[max_fd] = 1;

  while (1)
  {
    FD_ZERO(&fdset);
    FD_SET(STDIN_FILENO, &fdset);
    FD_SET(list_fd, &fdset);
    for (i_fd = 0; i_fd <= max_fd; i_fd++)
    if (fd_open[i_fd])
    FD_SET(i_fd, &fdset);

    printf("Choose 1 to connection management, 5 to quit\n");

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

      // setting as ready for reading
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

          // TODO: removing from list if crashed
          // TODO: if the list is empty must reconnect

          //updating max fd with the last open in fd_open
          if (max_fd == i_fd)
          {
            while (fd_open[--i_fd] == 0);
            max_fd = i_fd;
            break;
          }
          continue;
        }


        tid_args[tid_index] = i_fd;
        //request received correctly, switching between cases
        switch(request)
        {
          case NODE_CONNECTION:
          pthread_create(&tid[tid_index], NULL, node_connection, (void *)&tid_args[tid_index]);
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
          fprintf(stderr, "Request received is not correct\n");
          //print_menu();
          break;
        }
        tid_index++;
      }
    }
    for(int j=0; j < tid_index;  j++)
    pthread_join(tid[j], NULL);
  }
}
