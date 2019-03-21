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
static struct block getBlockFromNode(Tree node)
{
  struct block b = *(struct block *) node->info;
  return b;
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
    n_info = getBlockFromNode(tmp); // A
    sibl_info = getBlockFromNode(tmp->siblings); // B
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
  Tree tmp = blockchain->genesis;


  while(tmp->kids != NULL)
  {
    if (has_node_siblings(tmp))
      tmp = max_randtime(tmp);
    tmp = tmp->kids;
  }

  // if latest node of blockchain got siblings
  // use max_randtime to checkup the brother with max rand time
  // and then add block to him son;

  if(!has_node_siblings(tmp)) //if tmp hasn't brothers
  {
    if(compareBlockByInfo(tmp, block)) //if blocks got same hash
      create_sibling_to_node(tmp, block); // create new block as brother
    else
      create_kid_to_node(tmp, block); // else normally add to tail
  }
  else// there are multitail
  {
    fprintf(stderr,"Found more tails... choosing the one with max random time.\n");
    multitail = max_randtime(tmp);
    create_kid_to_node(multitail, block);
  }
  blockchain->b_size++;
  fprintf(stderr, "New block in blockchain\n");
}


// return block with that level in blockchain
Block searchByLevel(Blockchain blockchain, int level)
{
  Tree tmp = blockchain->genesis->kids;
  Block b = NULL;
  int i = 0;

  for(i = 1; i <= level; i++)
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

  if(a->n_block == b->n_block &&
      a->randomtime == b->randomtime &&
      compare_by_addr(a->info, b->info) )
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
