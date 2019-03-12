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
  printf("filled s_addr with %s:%hd",inet_ntoa(socket_address->sin_addr), ntohs(socket_address->sin_port));
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


bool compare_by_addr(void *x, void *y)
{
  struct new_conn_node a = *(struct new_conn_node *) x;
  struct new_conn_node b = *(struct new_conn_node *) y;
  printf("Comparing a->%s:%hu && b->%s:%hu", inet_ntoa(a.node->node_addr.sin_addr), ntohs(a.node->node_addr.sin_port), inet_ntoa(b.node->node_addr.sin_addr),ntohs(b.node->node_addr.sin_port));
  
  if( ( strncmp(inet_ntoa(a.node->node_addr.sin_addr), inet_ntoa(b.node->node_addr.sin_addr), LEN_ADDRESS) ) && 
      (ntohs(a.node->node_addr.sin_port) == ntohs(b.node->node_addr.sin_port)) )
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
