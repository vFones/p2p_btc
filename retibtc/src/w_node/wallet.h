#ifndef WALLET_H
#define WALLET_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <retibtc/list.h>
#include <retibtc/p2p.h>
#include <retibtc/net.h>
#include <retibtc/vitc.h>
#include <retibtc/sockwrap.h>


struct s_net_ent peer;
struct s_net_ent myent;
struct s_net_ent server;

struct sockaddr_in servaddr;
struct sockaddr_in peeraddr;

int servfd;
int peerfd;
char pwd[BUFFLEN];
float wallet_amount;

void usage(void);
void wallet_routine();

void menu_case(int choice);
void request_amount();
void send_coin();
void add_coin();

void request_case(short request);
void receive_transaction();

void hook2net();
void re_hook();
void server_authentication();

void print_menu();


#endif
