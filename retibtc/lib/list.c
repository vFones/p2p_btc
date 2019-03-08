#include "../include/list.h"

List create_list()
{
  List newlist = (List)Malloc(LIST);

  newlist->head = (Node)Malloc(NODE);
  newlist->head->info = NULL;
  newlist->head->next = NULL;
  newlist->tail = newlist->head;

  newlist->count = 0;

  return newlist;
}


int add_to_list(List l, void* info)
{
  if(l == NULL || info == NULL)
  {
    fprintf(stderr, "\nList, info to add or both are NULL\n");
    return -1;
  }

  Node newnode = (Node)Malloc(NODE);
  newnode->info = info;
  newnode->next = NULL;

  if(l->count == 0)
    l->head->next = newnode;
  else
    l->tail->next = newnode;

  l->tail = newnode;
  l->count++;

  return 0;
}


void empty_list(List l)
{
  Node marker = l->head->next;
  Node tmp = NULL;
  while(marker != NULL)
  {
    tmp = marker;
    marker = marker->next;
    free(tmp);
  }
  free(l->head);
  return;
}


void* extract_from_list(List l, void* info, COMPARE_NODE_INFO)
{
  if(l == NULL)
    return NULL;

  if(is_list_empty(l))
    return NULL;

  Node marker = l->head->next;
  Node tmp = marker;
  bool found;

  while( !(found = compare_node_info(marker->info, info)) && marker->next != NULL)
  {
    tmp = marker;
    marker = marker->next;
  }
  if(found)
  {
    l->count--;
    tmp->next = marker->next;
    if(marker == l->tail)
      l->tail = tmp;
    return marker->info;
  }
  return NULL;
}


bool remove_from_list(List l, void* info, COMPARE_NODE_INFO)
{
  Node marker = l->head->next;
  Node tmp = marker;
  bool found;

  while( !(found = compare_node_info(marker->info, info)) && marker->next != NULL)
  {
    tmp = marker;
    marker = marker->next;
  }
  if(found)
  {
    tmp->next = marker->next;
    l->count--;
    if(marker == l->tail)
      l->tail = tmp;
    free(marker);
  }
  return found;
}


bool is_list_empty(List l)
{
  if(l->count == 0)
    return true;
  return false;
}


int search_by_info(List l, void* info, COMPARE_NODE_INFO)
{
  if (is_list_empty(l))
    return -1;
  Node tmp = l->head->next;
  int i;

  if (tmp->info == NULL || info == NULL)
  {
    return -1;
  }
  i = 1;
  while(tmp != NULL)
  {
    if( compare_node_info(tmp->info, info) )
      return i;
    tmp = tmp->next;
    i++;
  }
  return -1;
}


void* search_by_index(List l, int index)
{
  Node tmp = l->head->next;
  int i = 0;

  if(index < 1 || index > l->count)
    return NULL;

  if(is_list_empty(l))
    return NULL;

  while( tmp != NULL && index != (++i) )
    tmp = tmp->next;

  return tmp->info;
}


void visit_list_of_list(Node n, VISIT_LIST, VISIT_NODE_INFO)
{

	if(n->next == NULL) return;

	visit_list(n->next->info, visit_node_info);
	visit_list_of_list(n->next, visit_list, visit_node_info);

}


void visit_list(void *args, VISIT_NODE_INFO)
{
  int i = 0;
  List l = (List) args;
  if(!is_list_empty(l))
  {
    Node tmp = l->head->next;
    while( tmp != NULL)
    {
      printf("[%d]\t", ++i);
      visit_node_info(tmp->info);
      tmp = tmp->next;
    }
    if(!i)
    fprintf(stderr,"List is empty\n");
  }
  return;
}


// EXAMPLE of implement
/*bool compare_info(void* x, void* y)
{
  int a = *(int*)x, b = *(int*)y;
  if(a == b)
    return true;
  return false;
}


void visit_block(void *args)
{
  Node n = (Node)args;
  printf("Intero: %d\n", *(int*)n->info);
  return;
}
*/
