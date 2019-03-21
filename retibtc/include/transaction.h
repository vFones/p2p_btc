#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "io.h"
#include "net.h"

typedef enum request{
  TRANSACTION,
  NODE_CONNECTION,
  WALLET_CONNECTION,
  BLOCK_SPREAD
} request_t;

typedef struct transaction{
  node_t src;
  node_t dst;
  float amount;
  int random;
} trns_t;
typedef struct transaction* Trns;
#define TRNS_SIZE sizeof(struct transaction)

void fillTransaction(node_t src, node_t dst, float amount, trns_t *t);
void visitTransaction(trns_t);

#endif