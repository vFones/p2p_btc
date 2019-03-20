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

short service_port;

Tree connected_node;
Tree connected_wallet;

int exit_flag;

int max_fd;
int *fd_open;

void n_routine();
void sig_handler(int sig_no);
void visitBlock(void* arg);

#endif
