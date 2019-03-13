#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "net.h"
#include "transaction.h"

struct block{
  int n_block;
  int randomtime;
  struct transaction trnsn;
};
typedef struct block* Block;
#define BLOCK_SIZE sizeof(struct block)

#endif
