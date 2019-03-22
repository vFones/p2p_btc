#include "../include/blockchain.h"


Blockchain create_blockchain()
{
  Block gen_block = (Block)Malloc(BLOCK_SIZE);

  fprintf(stderr, "\n**** Creating blockchain ****\n");

  gen_block->n_block = 0;
  gen_block->randomtime = 15;
  gen_block->info = NULL;

  Blockchain blockchain = (Blockchain)Malloc(BCHAIN_SIZE);

  blockchain->genesis = (Tree)Malloc(TREE_SIZE);
  blockchain->genesis->info = gen_block;
  blockchain->tail = blockchain->genesis;

  fprintf(stderr, "\n** genesis created **\n \n");
  return blockchain;
}


//used to get block from a specific node
static struct block getblockFromNode(Tree node)
{
  struct block b = *(struct block *) node->info;
  return b;
}

Block getBlockFromNode(Tree node)
{
  Block b = (Block)Malloc(BLOCK_SIZE);
  b = node ->info;
  return b;
}

static Tree max_randtime(Tree node)
{
  int tmpmax=0;
  struct block n_info, sibl_info;

  Tree found = NULL, tmp = NULL;
  found = node;
  tmp = node;

  fprintf(stderr, "Calculcating max random time \n");
  while(tmp->siblings != NULL)
  {
    n_info = getblockFromNode(tmp); // A
    sibl_info = getblockFromNode(tmp->siblings); // B
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
void addBlockToBlockchain(Blockchain blockchain, Block block)
{
  Tree multitail = NULL;
  Tree tmp = blockchain->tail;
  Tree last = NULL;
  Block new_block = (Block)Malloc(BLOCK_SIZE);
  memcpy(new_block, block, BLOCK_SIZE);

  // if latest node of blockchain got siblings
  // use max_randtime to checkup the brother with max rand time
  // and then add block to him son;

  if(!has_node_siblings(tmp)) //if tmp hasn't brothers
  {
    if(compareBlockByInfo(tmp, new_block)) //if blocks got same hash
    {
      last = create_sibling_to_node(tmp, new_block); // create new block as brother
    }
    else
    {
      last = create_kid_to_node(tmp, new_block); // else normally add to tail
      blockchain->b_size++;
    }
  }
  else// there are multitail
  {
    fprintf(stderr,"Found more tails... choosing the one with max random time.\n");
    multitail = max_randtime(tmp);
    last = create_kid_to_node(multitail, block);
    blockchain->b_size++;
  }
  blockchain->tail = last;
  fprintf(stderr, "New block [%d] in blockchain with size -> %d \n", block->n_block, blockchain->b_size);
}


// return block with that level in blockchain
Block searchByLevel(Blockchain blockchain, int level)
{
  Tree tmp = blockchain->genesis->kids;
  Block b = NULL;
  int i = 0;

  for(i = 1; i < level; i++)
  {
    if(tmp->kids != NULL)
    {
      //when going down the blockchain must consider multitails
      // so with max randtime we can "switch roads" until tails
      if (has_node_siblings(tmp))
        tmp = max_randtime(tmp);
      tmp = tmp->kids;
    }
  }

  //if (has_node_siblings(tmp))
  //  tmp = max_randtime(tmp);

  fprintf(stderr,"block-%d, found after %d iteration \n", level, i);

  b = (Block) tmp->info;
  return b;
}


bool compareBlockByInfo(void *x, void *y)
{
  Block a = (Block)x;
  Block b = (Block)y;
  Trns t_a = a->info;
  Trns t_b = b->info;

  if( a->n_block == b->n_block &&
      a->randomtime == b->randomtime &&
      compare_by_addr(&t_a->src, &t_b->src) &&
      compare_by_addr(&t_a->dst, &t_b->src))
    return true;
  return false;
}


int sendBlock(int fd, Block b)
{
  if(Write(fd, b, BLOCK_SIZE) != 0)
  {
    perror("sendBlock");
    return -1;
  }
  Trns t = NULL;
  t = b->info;
  Write(fd, t, TRNS_SIZE);

  return 0;
}


int recvBlock(int fd, Block b)
{
  if(Read(fd, b, BLOCK_SIZE) != 0)
  {
    perror("sendBlock");
    return -1;
  }
  Trns t = (Trns)Malloc(TRNS_SIZE);
  Read(fd, t, TRNS_SIZE);
  b->info = t;

  return 0;
}
