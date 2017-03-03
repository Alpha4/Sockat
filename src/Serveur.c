/*
*
* Serveur
* Author: Aurélien Brisseau & Sullivan Pineau
* Date: 03/03/2017 
*
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pthread.h>

#include "Structures.h"

#define MAX_CLIENTS 10

/***********************************************/
// Fonction permetant d'initialiser le serveur
/***********************************************/
void initialisation_du_serveur(info_connexion *informationServeur, int port)
{
    if((informationServeur->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Erreur pour créer un socket.");
        exit(1);
    }

    informationServeur->addresse.sin_family = AF_INET;
    informationServeur->addresse.sin_addr.s_addr = INADDR_ANY;
    informationServeur->addresse.sin_port = htons(port);

    if(bind(informationServeur->socket, (struct sockaddr *)&informationServeur->addresse, sizeof(informationServeur->addresse)) < 0)
    {
        perror("Erreur Liaison.");
        exit(1);
    }

    const int optVal = 1;
    const socklen_t optLen = sizeof(optVal);
    if(setsockopt(informationServeur->socket, SOL_SOCKET, SO_REUSEADDR, (void*) &optVal, optLen) < 0)
    {
        perror("Erreur d'option du socket.");
        exit(1);
    }


    if(listen(informationServeur->socket, 3) < 0) {
        perror("Erreur d'écoute.");
        exit(1);
    }
    printf("Le serveur SOCKAT est lancé...\n");
}


/***********************************************/
// Fonction permetant d'envoyer le message : texte à tous les utilisateurs/Clients
/***********************************************/
void envoi_message_public(info_connexion clients[], int emetteur, char *texte)
{
    message msg;
    msg.type = MESSAGE_PUBLIC;
    strncpy(msg.NomUtilisateur, clients[emetteur].NomUtilisateur, 20);
    strncpy(msg.donnees, texte, 256);
    int i = 0;
    for(i = 0; i < MAX_CLIENTS; i++)
    {
        if(i != emetteur && clients[i].socket != 0)
        {
            if(send(clients[i].socket, &msg, sizeof(msg), 0) < 0)
            {
                perror("Erreur d'envoi d'un message public.");
                exit(1);
            }
        }
    }
}

/***********************************************/
// Fonction permetant d'envoyer un message privée en provenance de : emetteur et à destination de : destinataire
/***********************************************/
void envoi_message_privee(info_connexion clients[], int emetteur,
    char *destinataire, char *texte)
{
    message msg;
    msg.type = MESSAGE_PRIVEE;
    strncpy(msg.NomUtilisateur, clients[emetteur].NomUtilisateur, 20);
    strncpy(msg.donnees, texte, 256);

    int i;
    for(i = 0; i < MAX_CLIENTS; i++)
    {
        if(i != emetteur && clients[i].socket != 0
            && strcmp(clients[i].NomUtilisateur, destinataire) == 0)
        {
            if(send(clients[i].socket, &msg, sizeof(msg), 0) < 0)
            {
                perror("Erreur d'envoi d'un message privée.");
                exit(1);
            }
            return;
        }
    }

    msg.type = UTILISATEUR_ERREUR;
    sprintf(msg.donnees, "L'utilisateur \"%s\" n'existe pas.", destinataire);
    printf(ROUGE "Un utilisateur a essayé de communiquer avec un utilisateur inconnu.\n" BLANC);

    if(send(clients[emetteur].socket, &msg, sizeof(msg), 0) < 0)
    {
        perror("Erreur d'envoi d'un message privée.");
        exit(1);
    }

}

/***********************************************/
// Fonction permetant de savoir si un Utilisateur peut se connecter et envoi sa réponse à celui-ci
/***********************************************/
void envoi_message_connexion(info_connexion *clients, int emetteur)
{
    message msg;
    msg.type = CONNEXION;
    strncpy(msg.NomUtilisateur, clients[emetteur].NomUtilisateur, 21);
    int i = 0;
    for(i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i].socket != 0)
        {
            if(i == emetteur)
            {
                msg.type = VALIDE;
                if(send(clients[i].socket, &msg, sizeof(msg), 0) < 0)
                {
                    perror("Erreur d'envoi d'un message connexion.");
                    exit(1);
                }
            }else
            {
                if(send(clients[i].socket, &msg, sizeof(msg), 0) < 0)
                {
                    perror("Erreur d'envoi d'un message connexion.");
                    exit(1);
                }
            }
        }
    }
}


/***********************************************/
// Fonction permetant d'effectuer une deconnexion 
/***********************************************/
void envoi_message_deconnexion(info_connexion *clients, char *NomUtilisateur)
{
    message msg;
    msg.type = DECONNEXION;
    strncpy(msg.NomUtilisateur, NomUtilisateur, 21);
    int i = 0;
    for(i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i].socket != 0)
        {
            if(send(clients[i].socket, &msg, sizeof(msg), 0) < 0)
            {
                perror("Erreur d'envoi d'un message déconnexion.");
                exit(1);
            }
        }
    }
}


/***********************************************/
// Fonction permetant d'envoyer la liste des utilisateurs connectés
/***********************************************/
void envoi_liste_utilisateurs(info_connexion *clients, int receiver) {
    message msg;
    msg.type = UTILISATEURS;
    char *list = msg.donnees;

    printf(JAUNE "Un utilisateur a demandé la liste de tous les utilisateurs connectés sur Sockat actuellement.\n" RESET);

    int i;

    list = stpcpy(list,"Voici la liste des utilisateurs connectés :");
    list = stpcpy(list, "\n");

    for(i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i].socket != 0)
        {
            list = stpcpy(list, clients[i].NomUtilisateur);
            list = stpcpy(list, "\n");
        }
    }
    list = stpcpy(list, "\n");

    if(send(clients[receiver].socket, &msg, sizeof(msg), 0) < 0)
    {
        perror("Erreur d'envoi d'un message de la liste d'utilisateurs.");
        exit(1);
    }

}


/***********************************************/
// Fonction permetant d'envoyer une réponse de serveur complet
/***********************************************/
void envoi_message_complet(int socket)
{
    message msgComplet;
    msgComplet.type = COMPLET;

    printf(ROUGE "Un utilisateur a tenté de se connecter mais le serveur est plein.\n" RESET);
    
    if(send(socket, &msgComplet, sizeof(msgComplet), 0) < 0)
    {
        perror("Erreur d'envoi d'un message complet.");
        exit(1);
    }

    close(socket);
}


/***********************************************/
// Fonction permetant de déconnecter tous les utilisateurs
/***********************************************/
void deconnexion(info_connexion connexion[])
{
    int i;
    for(i = 0; i < MAX_CLIENTS; i++)
    {
        close(connexion[i].socket);
    }
    exit(0);
}

/***********************************************/
// Fonction permetant d'analyser le message de l'utilisateur 
/***********************************************/
void analyse_message_utilisateur(info_connexion clients[], int emetteur)
{
    int read_size;
    message msg;

    if((read_size = recv(clients[emetteur].socket, &msg, sizeof(message), 0)) == 0)
    {
        printf(ROUGE "Un utilisateur s'est déconnecté: %s.\n" BLANC, clients[emetteur].NomUtilisateur);
        close(clients[emetteur].socket);
        clients[emetteur].socket = 0;
        envoi_message_deconnexion(clients, clients[emetteur].NomUtilisateur);

    } else {

        switch(msg.type)
        {
            case UTILISATEURS:
            envoi_liste_utilisateurs(clients, emetteur);
            break;

            case UTILISATEUR: ;
            int i;
            for(i = 0; i < MAX_CLIENTS; i++)
            {
                if(clients[i].socket != 0 && strcmp(clients[i].NomUtilisateur, msg.NomUtilisateur) == 0)
                {
                    close(clients[emetteur].socket);
                    clients[emetteur].socket = 0;
                    return;
                }
            }

            strcpy(clients[emetteur].NomUtilisateur, msg.NomUtilisateur);
            printf(VERT "Un utilisateur s'est connecté: %s\n" RESET, clients[emetteur].NomUtilisateur);
            envoi_message_connexion(clients, emetteur);
            break;

            case MESSAGE_PUBLIC:
            envoi_message_public(clients, emetteur, msg.donnees);
            printf(BLEU "Un message public a été reçu puis distribué à tout les utilisateurs.\n" RESET);
            break;

            case MESSAGE_PRIVEE:
            envoi_message_privee(clients, emetteur, msg.NomUtilisateur, msg.donnees);
            printf(CYAN "Un message privée a été reçu puis envoyé au destinataire.\n" RESET);
            break;

            default:
            fprintf(stderr, "Le message reçu est inconnu.\n");
            break;
        }
    }
}

//A définir !!!!

int construct_fd_set(fd_set *set, info_connexion *informationServeur,
    info_connexion clients[])
{
    FD_ZERO(set);
    FD_SET(STDIN_FILENO, set);
    FD_SET(informationServeur->socket, set);

    int max_fd = informationServeur->socket;
    int i;
    for(i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i].socket > 0)
        {
            FD_SET(clients[i].socket, set);
            if(clients[i].socket > max_fd)
            {
                max_fd = clients[i].socket;
            }
        }
    }
    return max_fd;
}


/***********************************************/
// Fonction permetant d'analyser si une nouvelle connexion est possible
/***********************************************/
void analyse_nouvelle_connexion(info_connexion *informationServeur, info_connexion clients[])
{
    int nouveau_socket;
    int longueur_adresse;
    nouveau_socket = accept(informationServeur->socket, (struct sockaddr*)&informationServeur->addresse, (socklen_t*)&longueur_adresse);

    if (nouveau_socket < 0)
    {
        perror("Erreur d'acceptation de nouveau socket.");
        exit(1);
    }

    int i;
    for(i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i].socket == 0) {
            clients[i].socket = nouveau_socket;
            break;

        } else if (i == MAX_CLIENTS -1)
        {
            envoi_message_complet(nouveau_socket);
        }
    }
}


/***********************************************/
// Fonction permetant d'analyser l'entrée de l'utilisateur
/***********************************************/
void analyse_entree_utilisateur(info_connexion clients[])
{
    char input[255];
    fgets(input, sizeof(input), stdin);
    sup_caractere_nouvelle_ligne(input);

    if(input[0] == 'q') {
        deconnexion(clients);
    }
}

/***********************************************/
// Main
/***********************************************/
int main(int argc, char *argv[])
{
    puts("SOCKAT est en cours de Chargement...");

    fd_set file_descriptors;

    info_connexion informationServeur;
    info_connexion clients[MAX_CLIENTS];

    int i;
    for(i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].socket = 0;
    }

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    initialisation_du_serveur(&informationServeur, atoi(argv[1]));

    while(true)
    {
        int max_fd = construct_fd_set(&file_descriptors, &informationServeur, clients);

        if(select(max_fd+1, &file_descriptors, NULL, NULL, NULL) < 0)
        {
            perror("Select Failed");
            deconnexion(clients);
        }

        if(FD_ISSET(STDIN_FILENO, &file_descriptors))
        {
            analyse_entree_utilisateur(clients);
        }

        if(FD_ISSET(informationServeur.socket, &file_descriptors))
        {
            analyse_nouvelle_connexion(&informationServeur, clients);
        }

        for(i = 0; i < MAX_CLIENTS; i++)
        {
            if(clients[i].socket > 0 && FD_ISSET(clients[i].socket, &file_descriptors))
            {
                analyse_message_utilisateur(clients, i);
            }
        }
    }

    return 0;
}
