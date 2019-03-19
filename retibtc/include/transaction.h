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
typedef struct transaction* Trns;
#define TRNS_SIZE sizeof(struct transaction)

Trns fillTransaction(
  struct connected_node src, \
  struct connected_node dst, \
  float amount );

int sendTrns(int fd, Trns trns);
int recvTrns(int fd, Trns trns);

#endif
