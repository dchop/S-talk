CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L 

all: build

build: list.o s-talk.o main.o
	gcc -std=c11 $(CFLAGS) list.o s-talk.o main.o -lpthread -o output

main.o: main.c
	gcc -c -g -W -Wall -Wpedantic main.c

list.o: list.c
	gcc -c -g -W -Wall -Wpedantic list.c

stalk-o: s-talk.c
	gcc -c -g -W -Wall -Wpedantic s-talk.c

run: build
	./output

valgrind: build
	valgrind --leak-check=full ./output

clean:
	rm -f *.o* *.out* output