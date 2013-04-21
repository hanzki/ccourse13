CFLAGS=-c -g -Wall -std=gnu99 -pedantic
LDFLAGS=
CC = gcc
LD = gcc
OBJS = dhtnode.o consoleui.o networker.o datastorage.o
PROG = dhtnode
LIBS = -lcrypto -lssl

.c.o:
	gcc $< -o $@ $(CFLAGS)

all: $(PROG)

$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $(PROG) $(LIBS)

dhtnode.o: dhtnode.c dhtnode.h dhtpacket.h consoleui.h 
	$(CC) $(CFLAGS) dhtnode.c

consoleui.o: consoleui.c consoleui.h
	$(CC) $(CFLAGS) consoleui.c
	
networker.o: networker.c networker.h dhtpacket.h
	$(CC) $(CFLAGS) networker.c
	
datastorage.o: datastorage.c datastorage.h dhtpacket.h
	$(CC) $(CFLAGS) datastorage.c
	
clean:
	/bin/rm -f *.o dhtnode

