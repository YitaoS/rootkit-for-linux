#include "my_malloc.h"
#include "assert.h"

#include <unistd.h>
#define node_size sizeof(node_t)

dlist_t freed_space = {NULL,NULL,0};//head,tail,usable freed size

unsigned long heap_size = 0;
/* functionality:  inplement first fit malloc */
/* input : the space size to malloc */
/* output : null - failure to malloc otherwise the new malloc pointer */
/* presumption: none postsumption: malloc a address with size of size Or return NULL*/

void * ff_malloc(size_t size) {
  if (size <= 0) {
    return NULL;
  }
  node_t * cur = find_first_usable_free_segment(size);
  if (cur == NULL) {
    cur = my_malloc(size);
    if (cur == NULL) {
      return NULL;
    }
  }
  else {
    malloc_freed_segment(cur, size);
  }
  return (void *)cur + node_size;
}

/* functionality:  inplement free operation */
/* input : the pointer that was malloc from ff_malloc*/
/* presumption: input is a valid address to free, postsumption: the matched addressis freed*/
void ff_free(void * ptr){
  if(ptr == NULL){
    return;
  }
  my_free((node_t *)(ptr - node_size));
}

/* functionality:  inplement best fit malloc */
/* input : the space size to malloc */
/* output : null - failure to malloc otherwise the new malloc pointer */
/* presumption: none postsumption: malloc a address with size of size Or return NULL*/
void * bf_malloc(size_t size) {
  if (size <= 0) {
    return NULL;
  }
  node_t * cur = find_best_usable_free_segment(size);
  if (cur == NULL) {
    cur = my_malloc(size);
    if (cur == NULL) {
      return NULL;
    }
  }
  else {
    malloc_freed_segment(cur, size);
  }
  return (void *)cur + node_size;
}

/* functionality:  inplement free operation */
/* input : the pointer that was malloc from bf_malloc*/
/* presumption: input is a valid address to free, postsumption: the matched addressis freed*/
void bf_free(void * ptr){
  if(ptr == NULL){
    return;
  }
  my_free((node_t *)(ptr - node_size));
}


/* functionality:  add node at the end of a dlist*/
/* input : ls - the dlist address. node - the node adress*/
/* presumption: add_node->next and ->prev are NULL, postsumption: ls is still a dlist but with node at the end*/
void add_node_at_tail(dlist_t * ls,node_t * node){
  if(ls->tail == NULL){
    ls->head = node;
    ls->tail = node;
  }else{
    ls->tail->next = node;
    ls->tail->next->prev = ls->tail;
    ls->tail = ls->tail->next;
  }
  ls->size += node->size+node_size;
}


/* functionality:  insert node and keep the order of address ascending*/
/* input : ls - the dlist address. node - the node adress*/
/* presumption: *ls is a dlist and the nodes address is ascending*/
/* postsumption: *ls is still a dlist and the nodes address is ascending with node inserted*/
void insert_node(dlist_t * ls,node_t * node){
  if(ls->head == NULL){
    ls->head = node;
    ls->tail = node;
  }else{
    node_t * cur = ls->head;
    if(cur>node){//before the head
      node->next = cur;
      cur->prev = node;
      ls->head = node;
    }else{
      while(cur->next!=NULL&&cur->next<node){
        cur = cur->next;
      }
      if(ls->tail == cur){
        cur->next = node;
        node->prev = cur;
        node->next = NULL;
        ls->tail = node;
      }else{
        node->next = cur->next;
        node->prev = cur;
        node->next->prev = node;
        node->prev->next = node;
      }
    }
  }
  ls->size += node->size+node_size;
}

/* functionality:  delete node and keep the order of address ascending*/
/* input : ls - the dlist address. node - the node adress*/
/* presumption: *ls is a dlist and the nodes address is ascending*/
/* postsumption: *ls is still a dlist and the nodes address is ascending without node deleted*/
void delete_node(dlist_t * ls, node_t * node){
  if(node->prev == NULL&&node->next==NULL){
    ls->head=NULL;
    ls->tail=NULL;
  }else if(node!=ls->head&&node!=ls->tail){
    node->next->prev = node->prev;
    node->prev->next = node->next;
    node->next = NULL;
    node->prev = NULL;
  }else if(node==ls->head){
    ls->head = ls->head->next;
    node->next->prev = node->prev;
    node->next = NULL;
    node->prev = NULL;
  }else{
    ls->tail = ls->tail->prev;
    node->prev->next = node->next;
    node->next = NULL;
    node->prev = NULL;
  }
  ls->size -= node->size + node_size;
}

/* functionality: malloc memory from new heap space */
/* input: the space size to malloc */
/* output : null - failure Otherwise the new malloc pointer */

node_t * my_malloc(size_t size) {
  void * cur = sbrk(node_size + size);
  heap_size += node_size + size;
  if (cur == (void *)-1) {
    return NULL;
  }
  node_t * node = (node_t *) cur;
  node->next = NULL;
  node->prev = NULL;
  node->size = size;
  return node;
}

// /* functionality: malloc space on freed segment from freed space, put left unused space into free space */
// /* input: cur - pointer to current freed segment size - space size to malloc */

void malloc_freed_segment(node_t * cur, size_t size) {
  //check if left space is enough for a node's metaspace , and make left space a new node for freed space
  //(if not, it cannot be reused ,so I will give all the space of this node to user)
  size_t left_size = cur->size - size;
  if(left_size > node_size){
    delete_node(&freed_space,cur); 
    cur->size = size;
    node_t * left_segment = (void*)cur + node_size + size;
    left_segment->size =  left_size - node_size;
    left_segment->next = NULL;
    left_segment->prev = NULL;
    insert_node(&freed_space,left_segment);
  }else{
    delete_node(&freed_space,cur);
  }
}


// /* functionality: 1.free assigned space and put them into freed space 2.converge it into consecutive unoccupied space */
// /* input: the address to free */

void my_free(node_t * cur) {
  //check if the address is NULL
  if (cur == NULL) {
    return;
  }
  insert_node(&freed_space,cur);
  if(cur->prev!=NULL&&cur->next!=NULL){
    if((void*)cur->prev+cur->prev->size+node_size == cur&&(void*)cur+cur->size+node_size == cur->next){
      cur->prev->size += node_size + cur->size+node_size + cur->next->size;
      delete_node(&freed_space,cur->next);
      delete_node(&freed_space,cur);
      return;
    }
  }
  if(cur->prev != NULL){
    if((void*)cur->prev+cur->prev->size+node_size == cur){
      cur->prev->size += node_size + cur->size;
      delete_node(&freed_space,cur);
      return;
    }
  }
  if(cur->next!=NULL){
    if((void*)cur+cur->size+node_size == cur->next){
      cur->size += node_size + cur->next->size;
      delete_node(&freed_space,cur->next);
    }
  }
}

//functionality: find the first_fit node that can be malloced
node_t * find_first_usable_free_segment(size_t size) {
  node_t * cur = freed_space.head;
  while (cur != NULL) {
    if (cur->size >= size) {
      return cur;
    }
    cur = cur->next;
  }
  return cur;
}

//functionality: find the best_fit node that can be malloced
node_t * find_best_usable_free_segment(size_t size) {
  node_t * cur = freed_space.head;
  node_t * ret = NULL;
  while (cur != NULL) {
    if (cur->size == size) {
      return cur;
    }
    if(cur->size > size){
      if(ret == NULL){
        ret = cur;
      }else{
        if(ret->size>cur->size){
          ret = cur;
        }
      }
    }
    cur = cur->next;
  }
  return ret;
}

//functionality: get the size of whole heap
unsigned long get_data_segment_size(){
  return heap_size;
}
//functionality: get the size of whole freed space
unsigned long get_data_segment_free_space_size(){
  if(freed_space.head == NULL){
    return 0;
  }
  unsigned long ans = 0;
  for(node_t*cur = freed_space.head;cur!=NULL;cur=cur->next){
    ans += cur->size+node_size;
  }
  return ans;
}


