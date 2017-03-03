CC = gcc
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)

all: Client Serveur

Client: Client.o Structures.o
	$(CC) $(LFLAGS) Client.o Structures.o -o obj/Client -lpthread

Serveur: Serveur.o Structures.o
	$(CC) $(LFLAGS) Serveur.o Structures.o -o obj/Serveur -lpthread


Client.o: src/Client.c src/Structures.h
	$(CC) $(CFLAGS) src/Client.c -lpthread

Serveur.o: src/Serveur.c src/Structures.h
	$(CC) $(CFLAGS) src/Serveur.c -lpthread

Structures.o: src/Structures.h src/Structures.c
	$(CC) $(CFLAGS) src/Structures.c -lpthread

clean:
	rm -rf *.o *~ obj/Client obj/Serveur 
