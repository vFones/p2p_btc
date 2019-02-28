#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>

#include <utils.h>


#define VISIT_LIST void(*visit_list)(void*, void(void*))
#define VISIT_NODE_INFO void(*visit_node_info)(void*)
#define COMPARE_NODE_INFO bool(*compare_node_info)(void*, void*)

#define EBADLIST 0
#define EBADNODE 1
#define EBADINDEX 2


struct s_node
{
	void* info;
	struct s_node* next;
};

struct s_list
{
  struct s_node* head;
  struct s_node* tail;
  int count;
};

typedef struct s_node* Node;
typedef struct s_list* List;

#define NODE sizeof(struct s_node)
#define LIST sizeof(struct s_list)

List create_list();
void empty_list(List l);
bool is_list_empty(List l);

int add_to_list(List l, void* info);
bool remove_from_list(List l, void* info, COMPARE_NODE_INFO);

void* extract_from_list(List l, void* info, COMPARE_NODE_INFO);

int search_by_info(List l, void* info, COMPARE_NODE_INFO);
void* search_by_index(List l, int index);

void visit_list_of_list(Node n, VISIT_LIST, VISIT_NODE_INFO);
void visit_list(void *args, VISIT_NODE_INFO);
//void visit_list(void *args);

// to implement
bool compare_node_info(void* x, void* y);
void visit_node_info(void *args);


#endif
