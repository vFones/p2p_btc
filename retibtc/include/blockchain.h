#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "io.h"
#include "net.h"
#include "tree.h"
#include "transaction.h"
#include <openssl/sha.h>

struct block{
  int n_block;
  int randomtime;
  node_t creator;
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
void addBlockToBlockchain(Blockchain blockchain, Block block);
Block getBlockFromNode(Tree);

Block searchByLevel(Blockchain blockchain, int level);

bool compareBlockByInfo(void *x, void *y);

int sendBlock(int fd, Block b);
int recvBlock(int fd, Block b);


#endif
