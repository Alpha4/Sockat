all:
	gcc src/Client.c -o obj/client -lpthread
	gcc src/Serveur.c -o obj/serv -lpthread

clean: 
	rm obj/client obj/serv