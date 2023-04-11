#include <stdlib.h>

//Best Fit malloc/free
void * ts_malloc_lock(size_t size);
void ts_free_lock(void * ptr);

void * ts_malloc_nolock(size_t size);
void ts_free_nolock(void * ptr);

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
node_t * find_best_usable_free_segment(size_t size);
node_t * my_malloc(size_t size);
void malloc_freed_segment(node_t * cur, size_t size);
void my_free(node_t * cur);

node_t * find_best_usable_free_segment_nolock(size_t size);
node_t * my_malloc_nolock(size_t size);
void malloc_freed_segment_nolock(node_t * cur, size_t size);
void my_free_nolock(node_t * cur);

void add_node_at_tail(dlist_t * ls,node_t * node);
void insert_node(dlist_t * ls,node_t * node);
void delete_node(dlist_t * ls, node_t * node);