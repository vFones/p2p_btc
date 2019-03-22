#ifndef MAIN_PROC_H
#define MAIN_PROC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // getopt
#include <errno.h>
#include <pthread.h>
#include <sys/param.h> // MAX
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/stat.h> // mkfifo
#include <fcntl.h> // open
#include <signal.h>

#include "../../include/io.h"
#include "../../include/net.h"
#include "../../include/tree.h"
#include "../../include/sockwrap.h"
#include "../../include/blockchain.h"
#include "../../include/transaction.h"

#define MSG "Usage: ./n_node -p <PORT TO EXPOSE>\n"

struct sockaddr_in my_server_addr;
node_t node_server;

node_t *node_list;
int node_list_size;

node_t *wallet_list;
int wallet_list_size;

pthread_rwlock_t node_mtx;
pthread_rwlockattr_t node_mtx_attr;

Blockchain blockchain;
pthread_rwlock_t bchain_mtx;
pthread_rwlockattr_t bchain_mtx_attr;

int exit_flag;
pthread_rwlock_t closed_flag;
pthread_rwlockattr_t closed_flag_attr;

pthread_t *tid;
int tid_index;
pthread_mutex_t t_index_mtx;
pthread_attr_t t_detached;


int max_fd;
pthread_rwlock_t fd_mtx;

void n_routine();
void sig_handler(int sig_no);
void visitBlock(void* arg);

#endif
