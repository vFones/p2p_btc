#ifndef MAIN_PROC_H
#define MAIN_PROC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // getopt
#include <errno.h>
#include <pthread.h>
#include <sys/param.h> // max
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "../../include/net.h"
#include "../../include/tree.h"
#include "../../include/sockwrap.h"
#include "../../include/transaction.h"

#define MSG "Usage: ./n_node -n node <service_port> -w <wallet_service port>\n"

short node_port;
short wallet_port;

struct sockaddr_in *node_info;
Tree connected_node;
Tree connected_wallet;


int max_fd;
int *fd_open;

void usage();
void n_routine();
void w_routine();

#endif
