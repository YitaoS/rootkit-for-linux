#include <stdlib.h>
#include <stdio.h>
#include "my_malloc.h"

#ifdef FF
#define MALLOC(sz) ff_malloc(sz)
#define FREE(p)    ff_free(p)
#endif
#ifdef BF
#define MALLOC(sz) bf_malloc(sz)
#define FREE(p)    bf_free(p)
#endif


int main(int argc, char *argv[])
{
  const unsigned NUM_ITEMS = 10;
  int i;
  int size;
  int sum = 0;
  int expected_sum = 0;
  int *array[NUM_ITEMS];

  size = 4;
  expected_sum += size * size;
  array[0] = (int *)MALLOC(size * sizeof(int));
  for (i=0; i < size; i++) {
    array[0][i] = size;
  } //for i
  for (i=0; i < size; i++) {
    sum += array[0][i];
  } //for i

  size = 16;
  expected_sum += size * size;
  array[1] = (int *)MALLOC(size * sizeof(int));
  for (i=0; i < size; i++) {
    array[1][i] = size;
  } //for i
  for (i=0; i < size; i++) {
    sum += array[1][i];
  } //for i

  size = 8;
  expected_sum += size * size;
  array[2] = (int *)MALLOC(size * sizeof(int));
  for (i=0; i < size; i++) {
    array[2][i] = size;
  } //for i
  for (i=0; i < size; i++) {
    sum += array[2][i];
  } //for i

  size = 32;
  expected_sum += size * size;
  array[3] = (int *)MALLOC(size * sizeof(int));
  for (i=0; i < size; i++) {
    array[3][i] = size;
  } //for i
  for (i=0; i < size; i++) {
    sum += array[3][i];
  } //for i

  FREE(array[0]);
  FREE(array[2]);

  size = 7;
  expected_sum += size * size;
  array[4] = (int *)MALLOC(size * sizeof(int));
  for (i=0; i < size; i++) {
    array[4][i] = size;
  } //for i
  for (i=0; i < size; i++) {
   sum += array[4][i];
  } //for i

  size = 256;
  expected_sum += size * size;
  array[5] = (int *)MALLOC(size * sizeof(int));
  for (i=0; i < size; i++) {
    array[5][i] = size;
  } //for i
  for (i=0; i < size; i++) {
    sum += array[5][i];
  } //for i

  FREE(array[5]);
  FREE(array[1]);
  FREE(array[3]);

  return 0;
}
