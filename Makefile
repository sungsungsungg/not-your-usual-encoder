CC=gcc -o3
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra
LDFLAGS=-lpthread

.PHONY: all
all: nyuenc

nyuenc: nyuenc.o

nyuenc.o: nyuenc.c


.PHONY: clean
clean:
	rm -f *.o nyuenc
