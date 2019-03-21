#include "../include/net.h"


void fillAddressIPv4(struct sockaddr_in *socket_address, char *ip_address, int port)
{
  memset((void *) socket_address, 0, sizeof(*socket_address));
  socket_address->sin_family = AF_INET;
  socket_address->sin_port = htons(port);
  if(ip_address != NULL)
  {
    if((inet_pton(AF_INET, ip_address, &(socket_address->sin_addr))) <= 0)
    {
      perror("Address creation error");
      exit(EXIT_FAILURE);
    }
  }
  else
    socket_address->sin_addr.s_addr = INADDR_ANY;
}


void getsockNode(int fd, node_t *node)
{
  struct sockaddr_in tmpaddr;
  socklen_t lentmpaddr = sizeof(tmpaddr);
  //Conn_node node = (Conn_node)Malloc(SIZE_NODE);
  if(getsockname(fd, (struct sockaddr *) &tmpaddr, &lentmpaddr) == -1)
  {
    perror("getsockname -> Node:");
    exit(EXIT_FAILURE);
  }
  strncpy(node->address, inet_ntoa(tmpaddr.sin_addr), LEN_ADDRESS);
  node->port = ntohs(tmpaddr.sin_port);
  node->fd = fd;
}


void getpeerNode(int fd, node_t *node)
{
  struct sockaddr_in tmpaddr;
  socklen_t lentmpaddr = sizeof(tmpaddr);
  if(getpeername(fd, (struct sockaddr *) &tmpaddr, &lentmpaddr) == -1)
  {
    perror("getpeername -> Node:");
    exit(EXIT_FAILURE);
  }
  strncpy(node->address, inet_ntoa(tmpaddr.sin_addr), LEN_ADDRESS);
  node->port = ntohs(tmpaddr.sin_port);
  node->fd = fd;
}


void visitConnectedNode(void *args)
{
  node_t *n = (node_t *)args;
  printf("Node = %s:%d in fd [%d]\n", n->address, n->port, n->fd);
}


void visitConnectedWallet(void *args)
{
  node_t *w = (node_t *) args;
  printf("Wallet = %s:%d in fd [%d]\n", w->address, w->port, w->fd);
}


bool compare_by_addr(void *x, void *y)
{
  node_t a = *(node_t *) x;
  node_t b = *(node_t *) y;

  if(!strncmp(a.address, b.address, LEN_ADDRESS) &&
     (a.port == b.port))
    return true;
  return false;
}


bool compare_by_fd(void *x, void *y)
{
  int a = *(int *) x, b = *(int *) y;
  if(a == b)
    return true;
  return false;
}

int choose_node(use_node_t *new_node)
{
  char buffer[BUFFLEN];

  printf("\nInsert a valid IPv4 address: ");
  scanf(" %s", buffer);
  strncpy(new_node->n.address, buffer, 32);

  printf("Insert a valid port address: ");
  scanf(" %d", &new_node->n.port);

  printf("Are those info correct? Press [y] to retry, any other char to skip node\n");
  scanf(" %c", &new_node->confirm);
  if(new_node->confirm == 'y')
    return 1;
  else
  {
    printf("\n***********\nInfo not correct\n***********\n");
    return 0;
  }
}
