CC=gcc
CFLAGS=-ggdb3 -fPIC
MALLOC_VERSION=FF
WDIR=..

all: my_malloc_test

my_malloc_test: my_malloc_test.c
	$(CC) $(CFLAGS) -I$(WDIR) -L$(WDIR) -D$(MALLOC_VERSION) -Wl,-rpath=$(WDIR) -o $@ my_malloc_test.c -lmymalloc -lrt

clean:
	rm -f *~ *.o my_malloc_test

clobber:
	rm -f *~ *.o
