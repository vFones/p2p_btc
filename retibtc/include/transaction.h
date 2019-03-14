#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "net.h"

enum node_macro{
  TRANSACTION,
  NODE_CONNECTION,
  WALLET_CONNECTION
};

struct transaction{
  char src[LEN_ADDRESS];
  short srcport;
  char dst[LEN_ADDRESS];
  short dstport;
  float amount;
  int random;
};
#define TRNS_SIZE sizeof(struct transaction);


#endif
