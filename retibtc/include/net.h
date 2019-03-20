#ifndef NET_H
#define NET_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>

#include "utils.h"

#define LEN_ADDRESS 32

typedef struct node_info{
  int fd;
  char address[LEN_ADDRESS];
  int port;
} node_t;
#define SIZE_NODE sizeof(struct connected_node)


typedef struct node_to_use{
  node_t n;
  char confirm;
} use_node_t;

void fillAddressIPv4(struct sockaddr_in *socket_address, char *ip_address, int port);

node_t getsockNode(int fd);
node_t getpeerNode(int fd);

void visitConnectedNode(void *args);
void visitConnectedWallet(void *args);
bool compare_connected_node(void *x, void *y);

bool compare_by_addr(void *x, void *y);
bool compare_by_fd(void *x, void *y);

int choose_node(use_node_t *new_node);

#endif
