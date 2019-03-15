#include "../include/net.h"


void fillAddressIPv4(struct sockaddr_in *socket_address, char *ip_address, unsigned short port)
{
  memset((void *) socket_address, 0, sizeof(*socket_address));
  socket_address->sin_family = AF_INET;
  socket_address->sin_port = htons(port);
  if(ip_address != NULL)
  {
    if ((inet_pton(AF_INET, ip_address, &(socket_address->sin_addr))) <= 0)
    {
      perror("Address creation error");
      exit(EXIT_FAILURE);
    }
  }
  else
    socket_address->sin_addr.s_addr = INADDR_ANY;
}


Conn_node getsockNode(int fd)
{
  struct sockaddr_in tmpaddr;
  socklen_t lentmpaddr = sizeof(tmpaddr);
  Conn_node node = (Conn_node)Malloc(SIZE_NODE);

  if(getsockname(fd, (struct sockaddr *) &tmpaddr, &lentmpaddr) == -1)
  {
    perror("getsockname -> Node:");
    return NULL;
  }
  strncpy(node->address, inet_ntoa(tmpaddr.sin_addr),LEN_ADDRESS);
  node->port = ntohs(tmpaddr.sin_port);
  node->fd = fd;

  return node;
}


Conn_node getpeerNode(int fd)
{
  struct sockaddr_in tmpaddr;
  socklen_t lentmpaddr = sizeof(tmpaddr);
  Conn_node node = (Conn_node)Malloc(SIZE_NODE);

  if( getpeername(fd, (struct sockaddr *) &tmpaddr, &lentmpaddr) == -1)
  {
    perror("getpeername -> Node:");
    return NULL;
  }
  strncpy(node->address, inet_ntoa(tmpaddr.sin_addr),LEN_ADDRESS);
  node->port = ntohs(tmpaddr.sin_port);
  node->fd = fd;

  return node;
}


void visitConnectedNode(void *args)
{
  Conn_node n = (Conn_node)args;
  printf("Node = %s:%hu in fd [%d]\n", n->address, n->port, n->fd);
}


bool compare_by_addr(void *x, void *y)
{
  Conn_node a = (Conn_node) x;
  Conn_node b = (Conn_node) y;

  if( !strncmp(a->address,b->address, LEN_ADDRESS) &&
      (a->port == b->port) )
    return true;
  return false;
}


bool compare_by_fd(void *x, void *y)
{
  int a = *(int*)x, b = *(int*)y;
  if(a == b)
      return true;
  return false;
}

struct confirm_new_node choose_node()
{
  //char buffer[256];
  char buffer[BUFFLEN];
  struct confirm_new_node new_node;

  new_node.node = (Conn_node)Malloc(SIZE_NODE);

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
