CC=gcc
LD=gcc
CFLAGS=-g
CPPFLAGS=-I. -I/home/cs437/exercises/ex3/include
SP_LIBRARY_DIR=/home/cs437/exercises/ex3

DEPS=structures.h

all: client server

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

client: $(SP_LIBRARY_DIR)/libspread-core.a client.o
	$(LD) -o $@ client.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a 

server:	$(SP_LIBRARY_DIR)/libspread-core.a server.o
	$(LD) -o $@ server.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a

clean: 
	rm -f *.o client
	rm -f *.o server
	


