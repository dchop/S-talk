CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L 

all: build

build: list.o
	gcc -std=c11 $(CFLAGS) list.o s-talk.c main.c -lpthread -o output

list.o: list.c
	gcc -c -g -W -Wall -Wpedantic list.c

run: build
	./output

valgrind: build
	valgrind --leak-check=full ./output

clean:
	rm -f output