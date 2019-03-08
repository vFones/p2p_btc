#ifndef TREE_H
#define TREE_H

#include "utils.h"

struct tree {
  struct tree *kids;
  struct tree *siblings;
  int depth, n_kids, n_sibl;
  void* info;
};
typedef struct tree* Tree;
#define TREE_SIZE sizeof(struct tree)

#define COMPARE_TREE_INFO bool(*compare_tree_info)(void*, void*)

Tree new_node(void *arg);
Tree add_sibling(Tree t, void *arg);
Tree add_kid(Tree t, void *arg);
Tree search_in_tree(Tree t, void *arg, COMPARE_TREE_INFO);



#endif
