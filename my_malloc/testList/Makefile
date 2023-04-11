test: testcasesList.c testList.o
	gcc -ggdb3 -fPIC -pedantic -Werror -Wall -o test testcasesList.c testList.o
testList.o: my_malloc.h testList.c
	gcc -ggdb3 -fPIC -pedantic -Werror -Wall -c testList.c
