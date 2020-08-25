CFLAGS = -std=c11 -g -W -Wall -Wpedantic
LIB = -lpthread 
MAIN = $(wildcard $(SRCDIR)/main.c)
LIST = $(wildcard $(SRCDIR)/list.c)
STALK = $(wildcard $(SRCDIR)/s-talk.c)
INC = -I include
BIN = bin/s-talk
all: clean build

build: list.o s-talk.o main.o
	@mkdir -p bin
	gcc $(CFLAGS) list.o s-talk.o main.o $(LIB) -o $(BIN)

main.o: $(MAIN)
	gcc -c -$(CFLAGS) $(INC) $(MAIN)

list.o: $(LIST)
	gcc -c $(CFLAGS) $(INC) $(LIST)

s-talk-o: $(STALK)
	gcc -c $(CFLAGS) $(INC) $(STALK)

mem: clean build mem-check

mem-check:
	@mkdir -p bin
	valgrind -s --leak-check=full \
			 --show-leak-kinds=all \
			 --track-origins=yes \
			 --show-reachable=yes\
			$(BIN) 6060 127.0.0.1 6001

clean:
	rm -f *.o* *.out* ./$(BIN)