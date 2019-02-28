#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include <utils.h>
#include <list.h>
#include <net.h>


#define VISIT_INFO void(*visit_info)(void*)
#define COMPARE_INFO bool(*compare_info)(void*, void*)

#define LEN_ID 120


struct s_info_block
{
  void *info; // info da decentralizzare tramite la block chain
  char id[LEN_ID]; // id del block
  int rand_sec; // secondi random da aspettare [5,10]
  struct s_net_ent creator;
  int confirmed; // flag che indica in caso di false che il blocco deve essere distrutto dal peer creator
};


typedef struct s_info_block *Info_block;
#define INFOBLOCK sizeof(struct s_info_block)


struct s_bc_block
{
  List block_list;
  char prev_id[LEN_ID]; // id del block precedente
  int seq; // numero di sequenza
};

typedef struct s_bc_block *BC_block;
#define BC_BLOCK sizeof(struct s_bc_block)

typedef List Bchain;
#define BCHAIN LIST

Bchain create_bchain();
void create_genesis_block(Bchain bc);

Info_block create_info_block(void *info, char id[LEN_ID], int r_sec, struct s_net_ent creator);

bool add_to_bchain(Bchain bc, Info_block info, int seq);
List extract_not_max_tails(Bchain bc);



bool is_bchain_empty(Bchain bc);
bool has_multi_tail(Bchain bc);

void empty_bchain(Bchain bc);


void visit_bchain(Bchain bc, VISIT_INFO);
void visit_list_info_block(List l, VISIT_INFO);
void visit_info_block(Info_block ib, VISIT_INFO);

bool compare_info_block(void *x, void *y);
bool compare_bc_block(void *x, void *y);

Info_block get_info_block_by_index(Bchain bc, int seq, int index);

bool get_prev_id(Bchain bc, char id[LEN_ID], int seq);

int get_tails_number(Bchain bc);
List get_tails(Bchain bc);
int get_info_block_number(Bchain bc);
void remove_tails(Bchain bc);
void extract_info_block(Bchain bc, int seq, Info_block ib);

List search_block_info(Bchain bc, void* info, COMPARE_INFO);
bool is_in_bchain(Bchain bc, Info_block ib, COMPARE_INFO);
#endif
