/*
*
* Client
* Author: Aurélien Brisseau & Sullivan Pineau
* Date: 03/03/2017 
*
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h> 


#include <sys/socket.h>
#include <arpa/inet.h>

#include "Structures.h"


/***********************************************/
// Fonction permetant de configurer le nom de l'utilisateur
/***********************************************/
void configuration_nomUtilisateur(char *NomUtilisateur)
{
	while(true)
	{
		printf("Enter un nom d'utilisateur: ");
		fflush(stdout);
		memset(NomUtilisateur, 0, 1000);
		fgets(NomUtilisateur, 22, stdin);
		sup_caractere_nouvelle_ligne(NomUtilisateur);

		if(strlen(NomUtilisateur) > 20)
		{
			puts("Le nom de l'utilisateur doit avoir au maximum 20 caractères !");

		} else {
			break;
		}
	}
}

/***********************************************/
// Fonction permetant d'envoyer au serveur le nouveau utilisateur au serveur 
/***********************************************/
void envoi_nouveau_utilisateur_au_serveur(info_connexion *connexion)
{
	message msg;
	msg.type = UTILISATEUR;
	strncpy(msg.NomUtilisateur, connexion->NomUtilisateur, 20);

	if(send(connexion->socket, (void*)&msg, sizeof(msg), 0) < 0)
	{
		perror("Erreur lors de l'envoi du nouveau utilisateur");
		exit(1);
	}
}

/***********************************************/
// Fonction permetant de se connecter au serveur
/***********************************************/
void connexion_au_serveur(info_connexion *connexion, char *addresse, char *port)
{

	while(true)
	{
		configuration_nomUtilisateur(connexion->NomUtilisateur);

		//Création du socket
		if ((connexion->socket = socket(AF_INET, SOCK_STREAM , IPPROTO_TCP)) < 0)
		{
			perror("erreur : impossible de creer la socket de connexion avec le serveur.");
			exit(1);
		}

		connexion->addresse.sin_addr.s_addr = inet_addr(addresse);
		connexion->addresse.sin_family = AF_INET;
		connexion->addresse.sin_port = htons(atoi(port));

		//Connexion au serveur
		if (connect(connexion->socket, (struct sockaddr *)&connexion->addresse , sizeof(connexion->addresse)) < 0)
		{
			perror("erreur : impossible de se connecter au serveur.");
			exit(1);
		}

		envoi_nouveau_utilisateur_au_serveur(connexion);

		message msg;
		ssize_t recv_val = recv(connexion->socket, &msg, sizeof(message), 0);
		if(recv_val < 0)
		{
			perror("Erreur reception");
			exit(1);

		}
		else if(recv_val == 0)
		{
			close(connexion->socket);
			printf("Ce nom d'utilisateur \"%s\" est déjà pris, veuillez en choisir un nouveau.\n", connexion->NomUtilisateur);
			continue;
		}

		break;
	}


	puts("Vous êtes désormais connecté à Sockat :)");
	puts("Pour afficher les informations, veuillez saisir /aide.");
}

/***********************************************/
// Fonction permetant de se deconnecter du serveur "normalement"
/***********************************************/
void deconnexion(info_connexion *connexion)
{
	close(connexion->socket);
	exit(0);
}

/***********************************************/
// Fonction permetant d'analyser le choix de l'utilsateur
/***********************************************/
void choix_utilisateur(info_connexion *connexion)
{
	char input[255];
	fgets(input, 255, stdin);
	sup_caractere_nouvelle_ligne(input);

	if(strcmp(input, "/q") == 0 || strcmp(input, "/quitter") == 0)
	{
		deconnexion(connexion);
	}
	else if(strcmp(input, "/t") == 0 || strcmp(input, "/tous") == 0)
	{
		message msg;
		msg.type = UTILISATEURS;

		if(send(connexion->socket, &msg, sizeof(message), 0) < 0)
		{
			perror("Erreur de l'envoi.");
			exit(1);
		}
	}
	else if(strcmp(input, "/aide") == 0)
	{
		puts("");
		puts(VIOLET "-------------------------------------------------------------------------------");
		puts(BLANC "/quitter or /q: Quitter Sockat.");
		puts("/aide: Afficher les informations.");
		puts("/tous ou /t: Voir tous les utilisateurs connectés sur Sockat.");
		puts("/p <Nom_du_destinaire> <message> Envoyer un message privée à Nom_du_destinaire.");
		puts(VIOLET "-------------------------------------------------------------------------------");
		puts("" RESET);
	}
	else if(strncmp(input, "/p", 2) == 0)
	{
		message msg;
		msg.type = MESSAGE_PRIVEE;

		char *Destinataire, *chatMsg;

		Destinataire = strtok(input+3, " ");

		if(Destinataire == NULL)
		{
			puts(ROUGE "Le format du message privé est : /m <Nom_du_destinaire> <message>" RESET);
			return;
		}

		if(strlen(Destinataire) == 0)
		{
			puts(ROUGE "Veuillez saisir un nom d'utilisateur." RESET);
			return;
		}

		if(strlen(Destinataire) > 20)
		{
			puts(ROUGE "Le nom d'utilisateur est compris entre 1 et 20 caractères." RESET);
			return;
		}

		chatMsg = strtok(NULL, "");

		if(chatMsg == NULL)
		{
			puts(ROUGE "Veuillez saisir un message valide." RESET);
			return;
		}

		strncpy(msg.NomUtilisateur, Destinataire, 20);
		strncpy(msg.donnees, chatMsg, 255);

		if(send(connexion->socket, &msg, sizeof(message), 0) < 0)
		{
			perror("Erreur de l'envoi.");
			exit(1);
		}

	}
	else
	{

		message msg;
		msg.type = MESSAGE_PUBLIC;
		strncpy(msg.NomUtilisateur, connexion->NomUtilisateur, 20);


		if(strlen(input) == 0) {
			return;
		}

		strncpy(msg.donnees, input, 255);

		if(send(connexion->socket, &msg, sizeof(message), 0) < 0)
		{
			perror("Erreur de l'envoi.");
			exit(1);
		}
	}



}

/***********************************************/
// Fonction permetant d'analyser le message reçu du serveur
/***********************************************/
void message_serveur_recu(info_connexion *connexion)
{
	time_t t = time(NULL); 
	char* date = asctime(localtime(&t));
	sup_caractere_nouvelle_ligne(date);

	message msg;
	ssize_t recv_val = recv(connexion->socket, &msg, sizeof(message), 0);
	if(recv_val < 0)
	{
		perror("Erreur de reception.");
		exit(1);

	}
	else if(recv_val == 0)
	{
		close(connexion->socket);
		puts("Le serveur Sockat est déconnecté.");
		exit(0);
	}

	switch(msg.type)
	{
		case CONNEXION:
		printf(VIOLET "%s - " JAUNE "%s s'est connecté sur Sockat." RESET "\n", date, msg.NomUtilisateur);
		break;

		case DECONNEXION:
		printf(VIOLET "%s - " JAUNE "%s s'est déconnecté de Sockat." RESET "\n", date, msg.NomUtilisateur);
		break;

		case UTILISATEURS:
		printf("%s", msg.donnees);
		break;

		case MESSAGE_PUBLIC:
		printf(VIOLET "%s - " BLANC "%s : %s" RESET "\n", date, msg.NomUtilisateur, msg.donnees);
		break;

		case MESSAGE_PRIVEE:
		printf(VIOLET "%s -" BLANC " de la part de %s :" CYAN " %s" RESET "\n", date, msg.NomUtilisateur, msg.donnees);
		break;

		case COMPLET:
		fprintf(stderr, VIOLET "%s -" ROUGE " Le serveur de Sockat est complet, veuillez résaissez ultérieurement." RESET "\n", date);
		exit(0);
		break;

		default:
		fprintf(stderr, VIOLET "%s -" ROUGE " Erreur : %s" RESET "\n", date, msg.donnees);
		break;
	}
}

/***********************************************/
// Main
/***********************************************/
int main(int argc, char *argv[])
{
	info_connexion connexion;
	fd_set file_descriptors;

	if (argc != 3) {
		fprintf(stderr,"Usage: %s <IP> <port>\n", argv[0]);
		fprintf(stderr,"(Si le problème persiste, veuillez essayer IP=127.0.0.1 PORT=5000)\n");
		exit(1);
	}

	connexion_au_serveur(&connexion, argv[1], argv[2]);

	while(true)
	{
		FD_ZERO(&file_descriptors);
		FD_SET(STDIN_FILENO, &file_descriptors);
		FD_SET(connexion.socket, &file_descriptors);
		fflush(stdin);

		if(select(connexion.socket+1, &file_descriptors, NULL, NULL, NULL) < 0)
		{
			perror("Erreur !!!");
			exit(1);
		}

		if(FD_ISSET(STDIN_FILENO, &file_descriptors))
		{
			choix_utilisateur(&connexion);
		}

		if(FD_ISSET(connexion.socket, &file_descriptors))
		{
			message_serveur_recu(&connexion);
		}
	}

	close(connexion.socket);
	return 0;
}
