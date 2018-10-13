CC=gcc
CFLAGS=-std=c99 -Wall -lm

SRCS = client.c server.c
BINARIES = $(basename $(SRCS))

PROGS = $(patsubst %.c,%,$(SRCS))

.PHONY: clean
.PHONY: all

all: server client

# %: %.c

#	$(CC) $(CFLAGS)  -o $@ $<

server: server.c list.c list.h

	$(CC) -o server server.c list.c

client: client.c
	
	$(CC) -o client client.c

clean:
	rm -f $(BINARIES)
