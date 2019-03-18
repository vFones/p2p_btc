#include "../include/blockchain.h"

Blockchain create_blockchain()
{
  struct block *gen_block = (Block)Malloc(BLOCK_SIZE);

  printf("creating blockchain: genesis\n");
  gen_block->prev_SHA256 = "0000000000000000000000000000000000000000000000000000000000000000";
  printf("prev_id: %s\n",gen_block->prev_SHA256);
  gen_block->SHA256 = "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f";
  printf("id: %s\n",gen_block->SHA256);
  gen_block->n_block = 0;
  gen_block->randomtime = 15;
  gen_block->info = NULL;

  Blockchain blockchain = (Blockchain)Malloc(BCHAIN_SIZE);

  blockchain->genesis = (Tree)Malloc(TREE_SIZE);
  blockchain->genesis->info = gen_block;
  blockchain->tail = blockchain->genesis;

  return blockchain;
}


//used to get block from a specific node
struct block getBlockFromNode(Tree node)
{
  struct block b = *(struct block *) node->info;
  return b;
}

char *getLatestSHA256(Blockchain blockchain)
{
  struct block *block = NULL;
  block = (struct block*)blockchain->tail->info;

  char *latest = block->SHA256;
  printf("gotLatestSHA256...\n");
  return latest;
}


static Tree max_randtime(Tree node)
{
  int tmpmax=0;
  struct block n_info, sibl_info;

  Tree found = NULL, tmp = NULL;
  found = node;
  tmp = node;

  while(tmp->siblings != NULL)
  {
    n_info = getBlockFromNode(tmp);
    sibl_info = getBlockFromNode(tmp->siblings);
    // A > B
    if(n_info.randomtime > sibl_info.randomtime)
    {
      if(n_info.randomtime > tmpmax)// A > FOUND
      {
        tmpmax = n_info.randomtime;
        found = tmp;
      }
    }
    else // B > A
    {
      if(sibl_info.randomtime > tmpmax)// B > FOUND
      {
        tmpmax = n_info.randomtime;
        found = tmp->siblings;
      }
    }
    tmp = tmp->siblings; // SHIFT A - B to B - C
  } // utill D = NULL
  return found; // return greatest
}


// add a new block to blockchain checking for multitail
void addBlockToBlockchain(Blockchain blockchain, struct block block)
{
  Tree tmp = blockchain->tail;
  Tree multitail = NULL;
  Tree new_son = NULL;
  // if latest node of blockchain got siblings
  // use max_randtime to checkup the brother with max rand time
  // and then add block to him son;

  if(!has_node_siblings(tmp))
  {
    new_son = create_kid_to_node(tmp, &block);
    blockchain->tail = new_son;
  }
  else// there are multitail
  {
    multitail = max_randtime(tmp);
    new_son = create_kid_to_node(multitail, &block);
    blockchain->tail = new_son;
  }
  blockchain->b_size++;
}


// return block with that level in blockchain
struct block searchByLevel(Blockchain blockchain, int level)
{
  Tree tmp = blockchain->genesis->kids;
  struct block b;
  int i = 0;

  while (tmp->kids != NULL || i < level)
  {
    tmp = tmp->kids;
    i++;
  }
  if (has_node_siblings(tmp))
    tmp = max_randtime(tmp);

  b = *(struct block *)(tmp->info);
  return b;
}