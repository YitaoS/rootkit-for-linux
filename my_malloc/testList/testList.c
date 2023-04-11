#include "my_malloc.h"

#include <unistd.h>
#define node_size sizeof(node_t)

dlist_t using_space = {NULL,NULL,0};//head,tail,using size
dlist_t freed_space = {NULL,NULL,0};//head,tail,usable freed size

unsigned long heap_size = 0;

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

void delete_node(dlist_t * ls, node_t * node){
  if(node!=ls->head&&node!=ls->tail){
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