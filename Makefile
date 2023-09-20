CC=gcc
LD=gcc

CFLAGS=-g
LFLAGS=-pthread -lcheck -lrt -lm

EXE=buddy

OBJS=main.o buddy_util.o bfree.o balloc.o

all: $(EXE)

$(EXE): $(OBJS)
	gcc $(LFLAGS) -o $(EXE) $^

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $^