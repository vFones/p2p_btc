#include "../include/transaction.h"

Trns fillTransaction( node_t src, node_t dst, float amount)
{
  Trns t = (Trns)Malloc(TRNS_SIZE);
  strncpy(t->src.address, src.address, LEN_ADDRESS);
  t->src.port = src.port;

  strncpy(t->dst.address, dst.address, LEN_ADDRESS);
  t->dst.port = dst.port;

  t->amount = amount;
  t->random = rand()%10000; // arbitrary number
  return t;
}

void visitTransaction(trns_t *t)
{
  fprintf(stderr, "[Random INT]->{%d} [src]://%s:%d --> [dst]://%s:%d [BTC]->%0.2F\n", t->random, t->src.address, t->src.port, \
    t->dst.address, t->dst.port, t->amount);
}
