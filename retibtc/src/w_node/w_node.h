#ifndef WALLET_H
#define WALLET_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../include/tree.h"
#include "../../include/net.h"
#include "../../include/utils.h"
#include "../../include/transaction.h"
#include "../../include/sockwrap.h"

#define MESSAGE "Usage: ./wallet [-n <node>] -p <node_port> \n"

struct connected_node node_info;
struct sockaddr_in node_address;

int sockfd;

struct connected_node wallet_info;
struct sockaddr_in wallet_address;

float wallet_amount;

void wallet_routine();

#endif
