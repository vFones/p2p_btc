#include "../include/tree.h"

Tree new_node(Tree parent, Tree prev_sibl, void *info)
{
  Tree new_tree = (Tree) Malloc(TREE_SIZE);

  if(parent != NULL)
  {
    new_tree->parent = parent;
    new_tree->depth = (parent->depth)+1;
  }
  else
  {
    new_tree->parent = NULL;
    new_tree->depth = 0;
  }

  if(prev_sibl != NULL)
    new_tree->prev_sibl = prev_sibl;
  else
    new_tree->prev_sibl = NULL;

  new_tree->siblings = NULL;
  new_tree->kids = NULL;

  if(info == NULL)
    new_tree->info = NULL;
  else
    new_tree->info = info;

  return new_tree;
}


Tree create_kid_to_node(Tree t, void *info)
{
  if (t == NULL)
    return NULL;

  // if T has kids then create kid as brother of T kid
  if (has_node_kids(t))
    return create_sibling_to_node(t->kids, info);
  else
    return (t->kids = new_node(t, NULL, info));
}


Tree create_sibling_to_node(Tree t, void *info)
{
  if(t == NULL)
    return NULL;

  while(t->siblings != NULL)
    t = t->siblings;

  t->siblings = new_node(t->parent, t, info);
  t->siblings->depth = t->siblings->prev_sibl->depth;
  return t->siblings;
}


bool add_sibling_to_node(Tree t, Tree to_add)
{
  if(t == NULL || to_add == NULL)
    return false;

  while(t->siblings != NULL)
    t = t->siblings;

  t->siblings = to_add;
  to_add->prev_sibl = t;
  return true;
}


bool add_kid_to_node(Tree t, Tree to_add)
{
  if (t == NULL || to_add == NULL)
    return false;

  if (t->kids != NULL)
     add_sibling_to_node(t->kids, to_add);
  else
  {
    t->kids = to_add;
    to_add->depth = (t->depth)+1;
  }
  return true;
}

Tree search_in_tree(Tree t, void *arg, COMPARE_TREE_INFO)
{
  if(t == NULL || arg == NULL)
    return NULL;

  if(t->info != NULL)
    if(compare_tree_info(t->info, arg))
      return t;

  if(search_in_tree(t->siblings, arg, compare_tree_info) != NULL)
    return t->siblings;

  return (search_in_tree(t->kids, arg, compare_tree_info));
}


void visit_tree(Tree t, VISIT_TREE_INFO)
{
  if(t == NULL)
    return;

  if(t->info != NULL)
    visit_tree_info(t->info);

  while(t->siblings != NULL)
  {
    visit_tree_info(t->siblings);
    t = t->siblings;
  }

  visit_tree(t->kids, visit_tree_info);
}


bool has_node_kids(Tree t)
{
  if(t != NULL)
    if(t->kids != NULL)
      return true;
  return false;
}


bool has_node_siblings(Tree t)
{
  if(t != NULL)
    if(t->siblings != NULL)
      return true;
  return false;
}

bool has_node_prev_sibl(Tree t)
{
  if(t != NULL)
    if(t->prev_sibl != NULL)
      return true;
  return false;
}

Tree remove_from_tree(Tree t, void *arg, COMPARE_TREE_INFO)
{
  if(t == NULL || arg == NULL)
    return NULL;

  Tree found = NULL;
  if( (found = search_in_tree(t, arg, compare_tree_info)) != NULL)
  {
    if(has_node_kids(found))
      add_kid_to_node(found->parent, found->kids);
    else
      found->parent->kids = NULL;

    if(has_node_prev_sibl(found))
    {
      if(has_node_siblings(found))
        add_sibling_to_node(found->prev_sibl, found->siblings);
      found->prev_sibl->siblings = NULL;
    }
  }
  return found;
}
