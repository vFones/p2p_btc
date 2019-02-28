#include <blockchain.h>

Bchain create_bchain()
{
  Bchain bc = create_list();
  return bc;
}

void create_genesis_block(Bchain bc)
{
  BC_block genesis = (BC_block)obj_malloc(BC_BLOCK);
  genesis->block_list = NULL;
  strncpy(genesis->prev_id, "NULL", LEN_ID);
  genesis->seq = 0;
  bc->head->info = (void *)genesis;
}


Info_block create_info_block(void *info, char id[LEN_ID], int r_sec, struct s_net_ent creator)
{
  Info_block ib = (Info_block)obj_malloc(INFOBLOCK);
  ib->info = info;
  strncpy(ib->id, id, LEN_ID);
  ib->rand_sec = r_sec;
  ib->creator = creator;
  ib->confirmed = 1;
  return ib;
}


bool add_to_bchain(Bchain bc, Info_block info, int seq)
{
  if(bc == NULL || info == NULL)
    return false;

  // puoi inserire o nel blocco successivo alla coda o nello stesso
  if(seq < bc->count || seq > bc->count + 1)
    return false;

  // se devo inserire un blocco con un nuovo seq
  // && la bc ha piu' code
  if(bc->count > 0)
    if(seq == bc->count+1 && has_multi_tail(bc))
      return false; // allora non lo posso fare perche' devo prima distruggere
                  // le code non valide

  // se stesso seq della coda
  if(seq == bc->count)
  { // allora multi-coda
    List tails = get_tails(bc);
    add_to_list(tails, info);
  }
  else //seq == bc->count+1
  { // devo creare un nuovo blocco della bchain vero e proprio
    BC_block new_bc_block = (BC_block)obj_malloc(BC_BLOCK);

    new_bc_block->block_list = create_list();
    new_bc_block->seq = seq;
    get_prev_id(bc, new_bc_block->prev_id, seq);

    //aggiungo il block ricevuto in input all BC_block appena creato
    add_to_list(new_bc_block->block_list, info);
    //aggiungo alla blockchain (lista di bc_block) il BC_block appena creato
    add_to_list(bc, new_bc_block);
  }
  return true;
}

List extract_not_max_tails(Bchain bc)
{
  if(!has_multi_tail(bc))
  {
    return NULL;
  }

  List block_to_remove = create_list();

  char max_id[LEN_ID];
  int max_sec = 0;

  List tails = get_tails(bc);
  int i;
  Info_block i_block;

  // search max tail
  for(i = 1; i <= tails->count; i++)
  {
    i_block = search_by_index(tails, i);
    if(i_block->rand_sec > max_sec)
      strncpy(max_id, i_block->id, LEN_ID);
  }
  // select not max tails
  for(i = 1; i <= tails->count; i++)
  {
    i_block = search_by_index(tails, i);

    if( strncmp(i_block->id, max_id, LEN_ID) != 0)
    {
      i_block->confirmed = 0;
      add_to_list(block_to_remove, i_block);
    }
  }
  // extract not max tails
  for(i = 1; i <= block_to_remove->count; i++ )
  {
    i_block = search_by_index(block_to_remove, i);
    extract_from_list(tails, i_block, compare_info_block);
  }
  return block_to_remove;
}

bool is_bchain_empty(Bchain bc)
{
  return (bc->count == 0 ? true : false);
}

bool has_multi_tail(Bchain bc)
{
  if(bc->count == 0)
    return false;

  List tails = get_tails(bc);

  return ( tails->count > 1 ? true : false);
}

void empty_bchain(Bchain bc)
{
  if(is_bchain_empty(bc))
    return;

  int i = 1;
  BC_block tmp = search_by_index(bc, i);

  while(tmp != NULL)
  {
    printf("\nempty BC_block seq{%d}\n", tmp->seq);
    empty_list(tmp->block_list);
    i++;
    tmp = search_by_index(bc, i);
  }
  empty_list(bc);
}

void visit_bchain(Bchain bc, VISIT_INFO)
{
  BC_block tmp = bc->head->info; // genesis
  int i=0;
  printf("SEQ = {%d}\n", i);
  printf("[GENESIS BLOCK]\n");
  printf("{VOID}\n\n");

  if(is_bchain_empty(bc))
  {
    printf("bchain empty\n");
    return;
  }
  // first BC_block
  i++;
  tmp = search_by_index(bc, i);
  while(tmp != NULL)
  {
    printf("\n*********************\n");
    printf("SEQ = {%d}\n", tmp->seq);
    printf("PREV ID-->%s\n", tmp->prev_id);
    visit_list_info_block(tmp->block_list, visit_info );
    i++;
    tmp = search_by_index(bc, i);
  }
}

void visit_list_info_block(List l, VISIT_INFO)
{
  if(is_list_empty(l))
  {
    printf("LIST OF INFO_BLOCK EMPTY!\n");
    return;
  }
  int i = 1;
  Info_block tmp = (Info_block)search_by_index(l, i);

  while(tmp != NULL)
  {
    printf("\n[%d]\n", i++);
    visit_info_block(tmp, visit_info);
    tmp = (Info_block)search_by_index(l, i);
  }
}

void visit_info_block(Info_block ib, VISIT_INFO)
{
  printf("\n#######\n");
  printf("INFO BLOCK ID ---> %s\n", ib->id);
  printf("CREATOR[:]--->[%s:%d]\n", ib->creator.addr, ib->creator.port);
  printf("WAITING TIME --->%d sec\n", ib->rand_sec);
  printf("CONFIRMED ---> %s\n\n", (ib->confirmed == 1) ? "TRUE" : "FALSE");
  visit_info(ib->info);
}

bool compare_info_block(void *x, void *y)
{
  Info_block a = (Info_block) x;
  Info_block b = (Info_block) y;

  if( strncmp(a->id, b->id, LEN_ID) == 0)
    return true;
  return false;
}

bool compare_bc_block(void *x, void *y)
{
  BC_block a =(BC_block)x;
  BC_block b =(BC_block)y;
  if(a->seq == b->seq && ( strncmp(a->prev_id, b->prev_id, LEN_ID) == 0))
    return true;
  return false;
}


Info_block get_info_block_by_index(Bchain bc, int seq, int index)
{
  if(seq > bc->count)
  {
    fprintf(stderr, "get_info_block_by_index: %s\n", "sequence number not valid");
    return NULL;
  }

  BC_block tmp = search_by_index(bc, seq);

  if(index > tmp->block_list->count)
  {
    fprintf(stderr, "get_info_block_by_index: %s\n", "index of Info_block not valid");
    return NULL;
  }

  return search_by_index(tmp->block_list, index);
}

int get_tails_number(Bchain bc)
{
  int n = 0;
  List tails = get_tails(bc);

  if(!is_bchain_empty(bc))
    n = tails->count;

  return n;
}

List get_tails(Bchain bc)
{
  if(!is_bchain_empty(bc))
  {
    BC_block tails = search_by_index(bc, bc->count);
    return tails->block_list;
  }
  return NULL;
}

int get_info_block_number(Bchain bc)
{
  // -1 necessario altrimenti una coda verrebbe contata 2 volte
  return bc->count + get_tails_number(bc) - 1;
}

void remove_tails(Bchain bc)
{
  if(is_bchain_empty(bc))
    return;

  BC_block tails = search_by_index(bc, bc->count);

  empty_list(tails->block_list);
  remove_from_list(bc, tails, compare_bc_block);
}

// e' possibile rimuovere solo una coda
void extract_info_block(Bchain bc, int seq, Info_block ib)
{
  if(bc == NULL || ib == NULL || seq != bc->count)
  {
    fprintf(stderr, "%s\n", "removal of Info_block not valid");
    return;
  }
  List tails = get_tails(bc);
  extract_from_list(tails, ib, compare_info_block);
}


//usata per cercare informazioni (transazioni) "uguali" secondo compare_info passata in input
List search_block_info(Bchain bc, void* info, COMPARE_INFO)
{
  if(is_bchain_empty(bc))
  {
    fprintf(stderr, "\n%s\n", "bchain empty");
    return NULL;
  }

  if(info == NULL)
  {
    fprintf(stderr, "\n%s\n", "info is invalid");
    return NULL;
  }


  List founded = create_list();
  int i , j;
  Info_block tmp_info_block;
  BC_block tmp_bc_block;

  i = 1;
  tmp_bc_block = search_by_index(bc, i);
  while(tmp_bc_block != NULL)
  {
    j = 1;
    tmp_info_block = search_by_index(tmp_bc_block->block_list, j);

    while(tmp_info_block != NULL)
    {
      if(compare_info(info, tmp_info_block->info) )
        add_to_list(founded, tmp_info_block->info);
      j++;
      tmp_info_block = search_by_index(tmp_bc_block->block_list, j);
    }
    i++;
    tmp_bc_block = search_by_index(bc, i);
  }

  if(founded->count == 0)
  {
    free(founded);
    return NULL;
  }

  return founded;
}

bool is_in_bchain(Bchain bc, Info_block ib, COMPARE_INFO)
{
  if(is_bchain_empty(bc))
  {
    fprintf(stderr, "\n%s\n", "bchain empty");
    return false;
  }


  List found = search_block_info(bc, ib->info, compare_info);

  return (found != NULL) ? true : false;
}


bool get_prev_id(Bchain bc, char id[LEN_ID], int seq)
{
  if(bc == NULL)
  {
    fprintf(stderr, "%s\n", "blockchain not valid");
    return false;
  }

  if(!is_bchain_empty(bc))
  {
    if(seq == 1)
    {
      strncpy(id, "NULL", LEN_ID);
      return true;
    }
    else
    {
      BC_block tmp = search_by_index(bc, seq-1);
      Info_block tmp_ib = search_by_index(tmp->block_list, 1);
      strncpy(id, tmp_ib->id, LEN_ID);
      return true;
    }
  }
  else // se la bchain e' vuota
  {
    strncpy(id, "NULL", LEN_ID);
    return false;
  }

}
