CC=gcc
LD=gcc
CFLAGS=-g
CPPFLAGS=-I. -I/home/rdeshpa3/ex3/include
SP_LIBRARY_DIR=/home/rdeshpa3/ex3

DEPS=structures.h

all: client server

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c ll_chat.c $<

client: $(SP_LIBRARY_DIR)/libspread-core.a client.o ll_chat.o
	$(LD) -o $@ client.o ll_chat.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a
	

server:	$(SP_LIBRARY_DIR)/libspread-core.a server.o ll_chat.o
	$(LD) -o $@ server.o ll_chat.o $(SP_LIBRARY_DIR)/libspread-core.a -ldl -lm -lrt -lnsl $(SP_LIBRARY_DIR)/libspread-util.a

clean: 
	rm -f *.o client
	rm -f *.o server
	


