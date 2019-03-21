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

void getsockNode(int fd, node_t *node);
void getpeerNode(int fd, node_t *node);

void visit_node_list(node_t n);
void visit_wallet_list(node_t n);

bool compare_connected_node(void *x, void *y);

bool compare_by_addr(void *x, void *y);
bool compare_by_fd(void *x, void *y);

int choose_node(use_node_t *new_node);

#endif
