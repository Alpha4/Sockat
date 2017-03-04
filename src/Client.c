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
void configuration_nom_utilisateur(char *nom_utilisateur)
{
	while(true)
	{
		printf("Enter un nom d'utilisateur: ");
		fflush(stdout);
		memset(nom_utilisateur, 0, 1000);
		fgets(nom_utilisateur, 22, stdin);
		sup_caractere_nouvelle_ligne(nom_utilisateur);

		if(strlen(nom_utilisateur) > 20)
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
	strncpy(msg.nom_utilisateur, connexion->nom_utilisateur, 20);

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
		configuration_nom_utilisateur(connexion->nom_utilisateur);

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
			printf("Ce nom d'utilisateur \"%s\" est déjà pris, veuillez en choisir un nouveau.\n", connexion->nom_utilisateur);
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
		puts(BLANC "_______________________________________________________________________________");
		puts("");
		puts("Tapez directement votre message pour l'envoyer à tout le monde.");
		puts( "-------------------------------------------------------------------------------");
		puts("/tous ou /t : Voir tous les utilisateurs connectés sur Sockat.");
		puts( "-------------------------------------------------------------------------------");
		puts("/p <Nom_du_destinaire> <message> : Envoyer un message privé à Nom_du_destinaire.");
		puts( "-------------------------------------------------------------------------------");
		puts("/c <Nom_du_groupe> : Créer un groupe privé. ");
		puts("/g <Nom_du_groupe> <message> : Envoyer un message à un groupe.");
		puts("/a <Nom_du_groupe> <Nom_Utilisateur> : Ajouter un utilisateur dans un groupe.");
		puts("-------------------------------------------------------------------------------");
		puts("/aide : Afficher les informations.");
		puts("/quitter or /q : Quitter Sockat.");
		puts("");
		puts("_______________________________________________________________________________");
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
			puts(ROUGE "Le format du message privé est : /p <Nom_du_destinaire> <message>" RESET);
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

		strncpy(msg.nom_utilisateur, Destinataire, 20);
		strncpy(msg.donnees, chatMsg, 255);

		if(send(connexion->socket, &msg, sizeof(message), 0) < 0)
		{
			perror("Erreur de l'envoi.");
			exit(1);
		}

	}
	else if(strncmp(input, "/a", 2) == 0)
	{
		message msg;
		msg.type = AJOUT_MEMBRE;

		char *nomDuGroupe, *nom_utilisateur;

		nomDuGroupe = strtok(input+3, " ");

		if(nomDuGroupe == NULL)
		{
			puts(ROUGE "Le format est : /a <Nom_du_groupe> <Nom Utilisateur>" RESET);
			return;
		}

		if(strlen(nomDuGroupe) > 20)
		{
			puts(ROUGE "Le nom du groupe est compris entre 1 et 20 caractères." RESET);
			return;
		}

		nom_utilisateur = strtok(NULL, "");

		if(nom_utilisateur == NULL || strlen(nom_utilisateur) > 20 )
		{
			puts(ROUGE "Veuillez saisir un utilisateur valide." RESET);
			return;
		}

		strncpy(msg.nom_utilisateur, nomDuGroupe, 20);
		strncpy(msg.donnees, nom_utilisateur, 20);

		if(send(connexion->socket, &msg, sizeof(message), 0) < 0)
		{
			perror("Erreur de l'envoi.");
			exit(1);
		}

	}
	else if(strncmp(input, "/c", 2) == 0){
		message msg;
		msg.type = CREER_GOUPE;

		char *nomDuGroupe;
		nomDuGroupe = strtok(input+3, " ");

		if(nomDuGroupe == NULL)
		{
			puts(ROUGE "Le format du est : /c <Nom_du_groupe>." RESET);
			return;
		}

		if(strlen(nomDuGroupe) > 20)
		{
			puts(ROUGE "Le nom du groupe est compris entre 1 et 20 caractères." RESET);
			return;
		}

		puts(BLEU "Demande de création d'un groupe ..." RESET);
		
		strncpy(msg.nom_utilisateur, nomDuGroupe, 20);

		if(send(connexion->socket, &msg, sizeof(message), 0) < 0)
		{
			perror("Erreur de l'envoi.");
			exit(1);
		}

	}
	else if(strncmp(input, "/g", 2) == 0)
	{
		message msg;
		msg.type = MESSAGE_GROUPE;

		char *nom_groupe, *chatMsg;

		nom_groupe = strtok(input+3, " ");

		if(nom_groupe == NULL)
		{
			puts(ROUGE "Le format du message de groupe est : /g <Nom_du_groupe> <message>" RESET);
			return;
		}

		if(strlen(nom_groupe) == 0)
		{
			puts(ROUGE "Veuillez saisir un nom de groupe." RESET);
			return;
		}

		if(strlen(nom_groupe) > 20)
		{
			puts(ROUGE "Le nom d'un groupe est compris entre 1 et 20 caractères." RESET);
			return;
		}

		chatMsg = strtok(NULL, "");

		if(chatMsg == NULL)
		{
			puts(ROUGE "Veuillez saisir un message valide." RESET);
			return;
		}

		strncpy(msg.nom_utilisateur, nom_groupe, 20);
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
		strncpy(msg.nom_utilisateur, connexion->nom_utilisateur, 20);


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
		printf(VIOLET "%s - " JAUNE "%s s'est connecté sur Sockat." RESET "\n", date, msg.nom_utilisateur);
		break;

		case DECONNEXION:
		printf(VIOLET "%s - " JAUNE "%s s'est déconnecté de Sockat." RESET "\n", date, msg.nom_utilisateur);
		break;

		case UTILISATEURS:
		printf("%s", msg.donnees);
		break;

		case MESSAGE_PUBLIC:
		printf(VIOLET "%s - " BLANC "%s : %s" RESET "\n", date, msg.nom_utilisateur, msg.donnees);
		break;

		case MESSAGE_PRIVEE:
		printf(VIOLET "%s -" BLANC " de la part de %s :" CYAN " %s" RESET "\n", date, msg.nom_utilisateur, msg.donnees);
		break;

		case MESSAGE_GROUPE:
		printf(VIOLET "%s -" BLANC " de la part du groupe %s :" CYAN " %s" RESET "\n", date, msg.nom_utilisateur, msg.donnees);
		break;

		case CREER_GOUPE_OK:
		printf(VIOLET "%s - " VERT "Votre groupe : %s a bien été ajouté." RESET "\n", date, msg.donnees);
		break;

		case AJOUT_MEMBRE_OK:
		printf(VIOLET "%s - " VERT "L'ajout du membre a bien été pris en compte." RESET "\n", date);
		break;

		case AJOUT_MEMBRE:
		printf(VIOLET "%s - " VERT "Vous avez été ajouté dans le groupe : %s." RESET "\n", date, msg.donnees);
		break;

		case AJOUT_MEMBRE_ERREUR:
		printf(VIOLET "%s - " ROUGE "L'ajout est impossible." RESET "\n", date);
		break;		

		case CREER_GOUPE_COMPLET:
		printf(VIOLET "%s - " ROUGE "impossible de créer votre groupe :%s, la limite du nombre de groupe a été atteint." RESET "\n", date, msg.donnees);
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
