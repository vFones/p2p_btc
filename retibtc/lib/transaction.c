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

void visitTransaction(trns_t t)
{
  printf("Transaction [%d]: [%s:%d] --> [%s:%d] [%0.2F]", t.random, t.src.address, t.src.port, \
    t.dst.address, t.dst.port, t.amount);
}