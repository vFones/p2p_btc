#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "tree.h"


struct block{
  char *prev_sha1;
  char *sha1;
  int n_block;
  int randomtime;
  void *info;
};
typedef struct block* Block;
#define BLOCK_SIZE sizeof(struct block)

struct blockchain{
  Tree genesis;
  Tree tail;
  int b_size;
};
typedef struct blockchain* Blockchain;
#define BCHAIN_SIZE sizeof(struct blockchain)


Blockchain create_blockchain();

//handling multitail
void addBlockToBlockchain(Blockchain blockchain, struct block block);

struct block getBlockFromNode(Tree node);

struct block searchByLevel(Blockchain blockchain, int level);

#endif
