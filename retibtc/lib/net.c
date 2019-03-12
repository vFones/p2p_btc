#include "../include/net.h"


void fillAddressIPv4(struct sockaddr_in *socket_address, char *ip_address, unsigned short port)
{
  memset((void *) socket_address, 0, sizeof(*socket_address));
  socket_address->sin_family = AF_INET;
  socket_address->sin_port = htons(port);
  printf("String ip: %s\n", ip_address);
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


Conn_node getConnectedNode(int fd, Conn_node node)
{
  struct sockaddr_in tmpaddr;
  socklen_t lentmpaddr = sizeof(tmpaddr);
  if( getpeername(fd, (struct sockaddr *) &tmpaddr, &lentmpaddr) == -1)
  {
    perror("getConnectedNode (getpeername)");
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
  printf("Node = %s:%hu\n", n->address, n->port);
}


bool compare_by_addr(void *x, void *y)
{
  Conn_node a = (Conn_node) x;
  Conn_node b = (Conn_node) y;
  fprintf(stderr,"Comparing a->%s:%hu && b->%s:%hu\n", \
    a->address, a->port, b->address, b->port);

  if( !strncmp(a->address,b->address, LEN_ADDRESS) &&
      (a->port == b->port) )
    return true;
  return false;
}

bool compare_by_fd(void *x, void *y)
{
  Conn_node a = (Conn_node) x;
  Conn_node b = (Conn_node) y;
  if(a->fd == b->fd)
      return true;
  return false;
}
