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


Conn_node getConnectedNode(int fd, Conn_node node)
{
  struct sockaddr_in tmpaddr;
  socklen_t lentmpaddr = sizeof(tmpaddr);
  if( getpeername(fd, (struct sockaddr *) &tmpaddr, &lentmpaddr) == -1)
  {
    perror("getConnectedNode (getpeername)");
    return NULL;
  }
  node->node_addr.sin_addr.s_addr = tmpaddr.sin_addr.s_addr;
  node->node_addr.sin_family = AF_INET;
  node->node_addr.sin_port = tmpaddr.sin_port;
  node->fd = fd;
  return node; // return 0 on success
}


void visitConnectedNode(void *args)
{
  Conn_node n = (Conn_node)args;
  printf("Node = %s:%hu\n", \
    inet_ntoa(n->node_addr.sin_addr), \
    ntohs(n->node_addr.sin_port));
}


bool compare_connected_node(void *x, void *y)
{
  Conn_node a = (Conn_node) x;
  Conn_node b = (Conn_node) y;
  if( (a->fd == b->fd) &&
  //TODO: check node_addr compare why is not working
      ( memcmp(&(a->node_addr), &(b->node_addr), sizeof(struct sockaddr_in)) == 0) &&
      ( ntohs(a->node_addr.sin_port) == ntohs(b->node_addr.sin_port) ) )
    return true;
  return false;
}
