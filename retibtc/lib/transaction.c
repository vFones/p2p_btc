#include "../include/transaction.h"

void fillTransaction( node_t src, node_t dst, float amount, trns_t *t)
{
  strncpy(t->src.address, src.address, LEN_ADDRESS);
  t->src.port = src.port;

  strncpy(t->dst.address, dst.address, LEN_ADDRESS);
  t->dst.port = dst.port;

  t->amount = amount;
  t->random = rand()%1000;
}

