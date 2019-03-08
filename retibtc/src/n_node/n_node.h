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
#include "../../include/p2p.h"
#include "../../include/list.h"
#include "../../include/sockwrap.h"


#define USAGE "Usage: ./n_node [-n node service port] [-w wallet service port]\n"

short node_port;
short wallet_port;


struct sockaddr_in *node_info;
List connected_node;

pthread_mutex_t mutex;
pthread_mutexattr_t mutexattr;

int max_fd;
int *fd_open;

void usage();
void n_routine();

#endif
