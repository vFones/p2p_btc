#include "../include/tree.h"

Tree new_node(void *arg)
{
    Tree new_tree = (Tree) Malloc(TREE_SIZE);

    new_tree->siblings = NULL;
    new_tree->n_sibl = 0;

    new_tree->kids = NULL;
    new_tree->n_kids = 0;

    new_tree->info = arg;
    new_tree->depth = 0;

    return new_tree;
}

Tree add_sibling(Tree t, void *arg)
{
    if (t == NULL)
      return NULL;

    while (t->siblings != NULL)
        t = t->siblings;

    t->n_sibl++;
    return (t->siblings = new_node(arg));
}

Tree add_kid(Tree t, void *arg)
{
    if ( t == NULL )
        return NULL;

    t->n_kids++;
    if ( t->kids != NULL )
      return add_sibling(t->kids, arg);
    else
    {
      t->depth++;
      return (t->kids = new_node(arg));
    }
}

Tree search_in_tree(Tree t, void *arg, COMPARE_TREE_INFO)
{
  if(t == NULL)
    return NULL;

  if(compare_tree_info(t->info, arg))
    return t;

  if(search_in_tree(t->siblings, arg, compare_tree_info)!=NULL)
    return t->siblings;

  return (search_in_tree(t->kids, arg, compare_tree_info));

}
