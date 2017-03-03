/*
*
* Structure.h
* Author: Aurélien Brisseau & Sullivan Pineau
* Date: 03/03/2017 
*
*/

#ifndef STRUCTURES_
#define STRUCTURES_

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define BLANC  "\x1B[37m"
#define ROUGE  "\x1B[31m"
#define CYAN  "\x1B[36m"
#define VIOLET  "\x1B[35m"
#define JAUNE  "\x1B[33m"
#define BLEU  "\x1B[34m"
#define VERT  "\x1B[32m"
#define RESET "\033[0m"

/***********************************************/
// Fonction permetant de supprimer le caracrtère de nouvelle ligne dans le texte
/***********************************************/
void sup_caractere_nouvelle_ligne(char *texte);

/***********************************************/
// Définition du type d'un message : CONNEXION, VALIDE, UTILISATEURS...
/***********************************************/
typedef enum
{
	CONNEXION,
	VALIDE,
	UTILISATEUR,
	UTILISATEURS,
	MESSAGE_PUBLIC,
	MESSAGE_PRIVEE,
	DECONNEXION,
	COMPLET,
	UTILISATEUR_ERREUR,
	ERREUR
} message_type;

/***********************************************/
// Définition de la structure d'un message 
/***********************************************/
typedef struct
{
	message_type type;
	char NomUtilisateur[21];
	char donnees[256];

} message;

/***********************************************/
// Définition de la structure des informations d'une connexion
/***********************************************/
typedef struct info_connexion
{
	int socket;
	struct sockaddr_in addresse;
	char NomUtilisateur[20];
} info_connexion;

#endif
