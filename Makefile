.PHONY:clean all

CC=gcc
CFLAGS=-Wall -g
BIN=srv cli
all:$(BIN)

%.o:%.c
	$(CC) $(CFLAGS) -c $< -o $@

cli:cli.o sckutil.o commsocket.o
	$(CC) $(CFLAGS) $^ -o $@

srv:srv.o sckutil.o commsocket.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf *.o $(BIN)

