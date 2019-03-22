#ifndef TREE_H
#define TREE_H

#include <stdbool.h>
#include "utils.h"

struct tree {
  struct tree *parent;
  struct tree *kids;
  struct tree *siblings;
  struct tree *prev_sibl;
  int depth;
  void* info;
};
typedef struct tree* Tree;
#define TREE_SIZE sizeof(struct tree)

#define COMPARE_TREE_INFO bool(*compare_tree_info)(void*, void*)
#define VISIT_TREE_INFO void(*visit_tree_info)(void*)

Tree new_node(Tree parent, Tree prev_sibl, void *info);

Tree create_kid_to_node(Tree t, void *info);
Tree create_sibling_to_node(Tree t, void *info);

bool add_sibling_to_node(Tree t, Tree to_add);
bool add_kid_to_node(Tree t, Tree to_add);

bool has_node_kids(Tree t);
bool has_node_siblings(Tree t);

Tree search_in_tree(Tree t, void *info, COMPARE_TREE_INFO);
void visit_tree(Tree t, VISIT_TREE_INFO);

Tree remove_from_tree(Tree t, void *info, COMPARE_TREE_INFO);

// to implement
bool compare_tree_info(void* x, void* y);
void visit_tree_info(void* arg);

#endif
