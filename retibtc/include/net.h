#ifndef NET_H
#define NET_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>

#include "io.h"

#define LEN_ADDRESS 32

struct connected_node{
  int fd;
  char address[LEN_ADDRESS];
  short port;
  char confirm;
};
#define CONN_NODE sizeof(struct connected_node)
typedef struct connected_node* Conn_node;


void fillAddressIPv4(struct sockaddr_in *socket_address, char *ip_address, \
  unsigned short port);

Conn_node getConnectedNode(int fd, Conn_node n);
void visitConnectedNode(void *args);
bool compare_connected_node(void *x, void *y);

bool compare_by_addr(void *x, void *y);
bool compare_by_fd(void *x, void *y);

#endif
