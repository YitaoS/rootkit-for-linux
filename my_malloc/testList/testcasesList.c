#include <stdlib.h>
#include <stdio.h>
#include "my_malloc.h"
#include <assert.h>

#ifdef FF
#define MALLOC(sz) ff_malloc(sz)
#define FREE(p)    ff_free(p)
#endif
#ifdef BF
#define MALLOC(sz) bf_malloc(sz)
#define FREE(p)    bf_free(p)
#endif

void testList(dlist_t * ls){
    node_t* cur = ls->head;
    assert(cur->prev==NULL);
    while(cur!=ls->tail){
        assert(cur->next->prev == cur);
        cur = cur->next;
    }
    cur=ls->tail;
    assert(cur->next==NULL);
    while(cur!=ls->head){
        assert(cur->prev->next == cur);
        cur = cur->prev;
    }
}
void print(dlist_t * ls){
  node_t * cur = ls->head;
  while(cur!=NULL){
    printf("%ld",cur->size);
    cur=cur->next;
  }
}
int main(int argc, char *argv[])
{
  printf("%ld\n",sizeof(node_t));
  dlist_t ls;
  dlist_t ls2 = {NULL,NULL,0};
  ls.head = NULL;
  ls.tail = NULL;
  ls.size = 0;
  node_t * node[9];
  for(int i = 0;i < 9;i++){
    node[i] = malloc(sizeof(node_t));
    node[i]->size = i;
    node[i]->next = NULL;
    node[i]->prev = NULL;
    ls.size-=24;
    insert_node(&ls,node[i]);
    testList(&ls);
  }
  for(int i = 0;i < 9;i++)testList(&ls);
  for(int i= 0; i < 9; i+=2){
    delete_node(&ls,node[i]);
    insert_node(&ls2,node[i]);
    testList(&ls);
    testList(&ls2);
  }
  print(&ls);
  print(&ls2);

  for(int i = 0;i < 9;i++){
    free(node[i]);
  }
  return EXIT_SUCCESS;
}
