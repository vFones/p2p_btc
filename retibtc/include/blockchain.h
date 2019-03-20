#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include "io.h"
#include "tree.h"
#include "transaction.h"
#include <openssl/sha.h>

struct block{
  unsigned char prev_SHA256[SHA256_DIGEST_LENGTH];
  unsigned char SHA256[SHA256_DIGEST_LENGTH];
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
void addBlockToBlockchain(Blockchain blockchain, Block block);

void getLatestSHA256(Blockchain blockchain, unsigned char *SHA);

Block searchByLevel(Blockchain blockchain, int level);

bool compareBlockByInfo(void *x, void *y);

int sendBlock(int fd, Block b);
int recvBlock(int fd, Block b);


#endif
