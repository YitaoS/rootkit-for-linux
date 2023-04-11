#include <stdlib.h>
//First Fit malloc/free
void * ff_malloc(size_t size);
void ff_free(void * ptr);
//Best Fit malloc/free
void * bf_malloc(size_t size);
void bf_free(void * ptr);


unsigned long get_data_segment_size(); //in bytes
unsigned long get_data_segment_free_space_size(); //in byte

typedef struct node {
  struct node * next;
  struct node * prev;
  size_t size;
} node_t;

typedef struct dlist{
  node_t * head;
  node_t * tail;
  unsigned long size;
}dlist_t;

//helper functions
node_t * find_first_usable_free_segment(size_t size);
node_t * find_best_usable_free_segment(size_t size);
node_t * my_malloc(size_t size);
void malloc_freed_segment(node_t * cur, size_t size);
node_t * find_prev_freed_node(node_t * cur);
void my_free(node_t * cur);

void add_node_at_tail(dlist_t * ls,node_t * node);
void insert_node(dlist_t * ls,node_t * node);
void delete_node(dlist_t * ls, node_t * node);