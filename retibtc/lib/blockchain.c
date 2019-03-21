#include "../include/blockchain.h"

Blockchain create_blockchain()
{
  Block gen_block = (Block)Malloc(BLOCK_SIZE);

  printf("\n**** Creating blockchain ****\n");

  gen_block->n_block = 0;
  gen_block->randomtime = 15;
  gen_block->info = NULL;

  Blockchain blockchain = (Blockchain)Malloc(BCHAIN_SIZE);

  blockchain->genesis = (Tree)Malloc(TREE_SIZE);
  blockchain->genesis->info = gen_block;
  blockchain->tail = blockchain->genesis;

  printf("\n** genesis created **\n \n");
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
void addBlockToBlockchain(Blockchain blockchain, Block block)
{
  Tree multitail = NULL;
  Tree new_son = NULL;
  // if latest node of blockchain got siblings
  // use max_randtime to checkup the brother with max rand time
  // and then add block to him son;
  printf("\n\nAggiungo alla blockchain \n");
  if(!has_node_siblings(blockchain->tail)) //if tmp hasn't brothers
  {
    printf("La tail non ha nessun fratello\n");
    if(compareBlockByInfo(blockchain->tail, block)) //if blocks got same hash
    {
      printf("Il blocco esiste già quindi creo blocco come fratello\n");
      new_son = create_sibling_to_node(blockchain->tail, block); // create new block as brother
    }
    else
    {
      printf("Il blocco non esiste quindi creo come figlio\n");
      new_son = create_kid_to_node(blockchain->tail, block); // else normally add to tail
    }
  }
  else// there are multitail
  {
    printf("la tail ha fratelli quindi scelgo quello con max rand time\n");
    multitail = max_randtime(blockchain->tail);
    new_son = create_kid_to_node(multitail, block);
  }
  blockchain->tail = new_son;
  blockchain->b_size++;
  printf("Aggiorno la tail e size blockchain [%d]\n\n", blockchain->b_size);
}

// return block with that level in blockchain
Block searchByLevel(Blockchain blockchain, int level)
{
  Tree tmp = blockchain->genesis->kids;
  Block b = NULL;
  int i = 1;

  for(i = 1; i < level; i++)
  {
    if(tmp->kids != NULL)
      tmp = tmp->kids;
  }

  if (has_node_siblings(tmp))
    tmp = max_randtime(tmp);

  printf("Level: %d, found at level: %d", level, i);
  
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
