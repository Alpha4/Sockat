CC = gcc
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)

all: Client Serveur

Client: obj/Client.o obj/Structures.o
	$(CC) $(LFLAGS) obj/Client.o obj/Structures.o -o bin/Client -lpthread

Serveur: obj/Serveur.o obj/Structures.o
	$(CC) $(LFLAGS) obj/Serveur.o obj/Structures.o -o bin/Serveur -lpthread


obj/Client.o: src/Client.c src/Structures.h
	$(CC) $(CFLAGS) -o obj/Client.o src/Client.c -lpthread

obj/Serveur.o: src/Serveur.c src/Structures.h
	$(CC) $(CFLAGS) -o obj/Serveur.o src/Serveur.c -lpthread

obj/Structures.o: src/Structures.h src/Structures.c
	$(CC) $(CFLAGS) -o obj/Structures.o src/Structures.c -lpthread

clean:
	rm -rf obj/*.o *~ bin/Client bin/Serveur 
