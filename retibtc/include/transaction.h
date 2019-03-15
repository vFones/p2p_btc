#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "net.h"

enum node_macro{
  TRANSACTION,
  NODE_CONNECTION,
  WALLET_CONNECTION
};

struct transaction{
  struct connected_node src;
  struct connected_node dst;
  float amount;
  int random;
};
#define TRNS_SIZE sizeof(struct transaction);

struct transaction fillTransaction(
  struct connected_node src, \
  struct connected_node dst, \
  float amount );

int sendTrns(int fd, struct transaction trns);
int recvTrns(int fd, struct transaction trns);


#endif
