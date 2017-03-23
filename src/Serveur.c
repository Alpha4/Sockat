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
// Fonction permetant de savoir si un groupe existe déjà
/***********************************************/
bool groupe_existant(char *nom_groupe, info_groupe groupes[]){
    int i;
    for(i=0; i<MAX_GROUPES; ++i){
        if(strcmp(groupes[i].nom_groupe,nom_groupe) == 0){
            return true;
        }
    }
    return false;
}

/***********************************************/
// Fonction permetant de savoir si un utilisateur appartient à un groupe
/***********************************************/
bool appartient_groupe(info_connexion client, info_groupe groupe){
    int i;
    for(i=0; i<groupe.nombre_membres; ++i){
        if(strcmp(client.nom_utilisateur,groupe.membres[i].nom_utilisateur) == 0){
            return true;
        }
    }
    return false;
}

/***********************************************/
// Fonction permetant de synchroniser client dans le groupe
/***********************************************/
void synchronise_groupe(info_connexion client, info_groupe *groupe){
    if(appartient_groupe(client,*groupe)){
        bool trouve = false;
        bool createur = false;
        int i;
        for(i=0; i<groupe->nombre_membres-1; ++i){
            if(strcmp(groupe->membres[i].nom_utilisateur, client.nom_utilisateur)==0){
                trouve=true;
                if(i==0){
                    createur=true;
                }
            }
            if(trouve){
                if(i<groupe->nombre_membres){
                    groupe->membres[i]=groupe->membres[i+1];
                }
            }
        }
        groupe->nombre_membres=groupe->nombre_membres-1;
        if(createur){
            message msg;
            msg.type = CREATEUR_GROUPE;
            strncpy(msg.nom_utilisateur, groupe->nom_groupe, 20);
            if(send(groupe->membres[0].socket, &msg, sizeof(msg), 0) < 0)
            {
                perror("Erreur d'envoi d'un message créateur.");
                exit(1);
            }
        }
    }
}



/***********************************************/
// Fonction permetant de synchroniser client dans tous les groupes
/***********************************************/
void synchronise_groupes(info_connexion client, info_groupe groupes[]){
    int i = 0;
    for(i=0; i<MAX_GROUPES; ++i){
        synchronise_groupe(client, &groupes[i]);
    }
}


/***********************************************/
// Fonction permetant d'envoyer le message : texte à tous les utilisateurs/Clients
/***********************************************/
void envoi_message_public(info_connexion clients[], int emetteur, char *texte)
{
    message msg;
    msg.type = MESSAGE_PUBLIC;
    strncpy(msg.nom_utilisateur, clients[emetteur].nom_utilisateur, 20);
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
    printf(CYAN "Sleep de 2 secondes en cours pour un thread ...\n" RESET);
    sleep(2);//Permet de visualiser le bon fonctionnemement des threads 

    message msg;
    msg.type = MESSAGE_PRIVEE;
    strncpy(msg.nom_utilisateur, clients[emetteur].nom_utilisateur, 20);
    strncpy(msg.donnees, texte, 256);

    int i;
    for(i = 0; i < MAX_CLIENTS; i++)
    {
        if(i != emetteur && clients[i].socket != 0
            && strcmp(clients[i].nom_utilisateur, destinataire) == 0)
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
    strncpy(msg.nom_utilisateur, clients[emetteur].nom_utilisateur, 21);
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
void envoi_message_deconnexion(info_connexion *clients, char *nom_utilisateur)
{
    message msg;
    msg.type = DECONNEXION;
    strncpy(msg.nom_utilisateur, nom_utilisateur, 21);
    int i = 0;
    for(i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i].socket != 0)
        {
            if(send(clients[i].socket, &msg, sizeof(msg), 0) < 0)
            {
                perror("Erreur d'envoi d'un message déconnexion.");
                // exit(1);
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
            list = stpcpy(list, clients[i].nom_utilisateur);
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
// Fonction permetant de créer un groupe
/***********************************************/
void creer_groupe(info_connexion clients[], info_groupe groupes[], int emetteur, char *nom_groupe){
    int groupeLibre=0;
    int i=0;
    bool libre = false;
    while(!libre && i<MAX_GROUPES){
        if(groupes[i].nombre_membres == 0){
            libre = true;
            groupeLibre = i;
        }
        ++i;
    }
    if(groupe_existant(nom_groupe, groupes)){
        libre=false;
    }

    message msg;
    strncpy(msg.donnees, nom_groupe, 20);

    if(!libre){
        msg.type = CREER_GOUPE_ERREUR;
        printf(ROUGE "Erreur de création de groupe : la limite du nombre de groupe a été atteint ou groupe déjà existant.\n" RESET);
    }else{
        groupes[groupeLibre].membres[0]=clients[emetteur];
        groupes[groupeLibre].nombre_membres = 1;
        strncpy(groupes[groupeLibre].nom_groupe, nom_groupe, 20);
        msg.type = CREER_GOUPE_OK;
        printf(VERT "%s a créé le groupe %s.\n" RESET, clients[emetteur].nom_utilisateur, nom_groupe);

    }

    if(send(clients[emetteur].socket, &msg, sizeof(msg), 0) < 0)
    {
        perror("Erreur d'envoi d'un message de la liste d'utilisateurs.");
        exit(1);
    }
}

/***********************************************/
// Fonction permetant de ajouter un membre dans un groupe
/***********************************************/
void ajout_membre_groupe(info_connexion clients[], info_groupe groupes[], int emetteur, char *nom_groupe, char *donnees){
    int i = 0;
    bool trouve = false;
    int number_groupe = 0;
    while(!trouve && i < MAX_GROUPES){
        if(groupes[i].nombre_membres>0){
            if(strcmp(groupes[i].nom_groupe, nom_groupe) == 0 ){
                trouve = true;
                number_groupe = i;
            }
        }
        ++i;
    }
    message msg;
    strncpy(msg.donnees, nom_groupe, 20);

    if(!trouve){
        printf(ROUGE "Un utilisateur a tenté d'ajouter un utilisateur dans un groupe inexistant.\n" RESET);
        msg.type = AJOUT_MEMBRE_ERREUR;
    }else{
        if(strcmp(groupes[number_groupe].membres[0].nom_utilisateur,clients[emetteur].nom_utilisateur)){
            printf(ROUGE "Un utilisateur a tenté d'ajouter un utilisateur dans un groupe sans être le créateur du groupe.\n" RESET);
            msg.type = AJOUT_MEMBRE_ERREUR;
        }else{
            trouve = false;
            i = 0;
            int number_client = 0;
            while(!trouve && i < MAX_CLIENTS){
                if(strcmp(clients[i].nom_utilisateur,donnees)==0){
                    trouve = true;
                    number_client=i;
                }
                ++i;
            }
            if(!trouve){
                printf(ROUGE "Un utilisateur a tenté d'ajouter un utilisateur inexistant dans un groupe.\n" RESET);
                msg.type = AJOUT_MEMBRE_ERREUR;
            }else{
                int nb_membres = groupes[number_groupe].nombre_membres; 
                if(nb_membres>=MAX_MEMBRE_PAR_GROUPE){
                    printf(ROUGE "Un utilisateur a tenté d'ajouter un utilisateur dans un groupe complet.\n" RESET);
                    msg.type = AJOUT_MEMBRE_ERREUR;
                }else{
                    if(appartient_groupe(clients[number_client],groupes[number_groupe])){
                        printf(ROUGE "Un utilisateur a tenté d'ajouter un utilisateur dans un groupe déjà existant.\n" RESET);
                        msg.type = AJOUT_MEMBRE_ERREUR;
                    }else{
                        groupes[number_groupe].membres[nb_membres] = clients[number_client];
                        groupes[number_groupe].nombre_membres++;
                        msg.type = AJOUT_MEMBRE_OK;
                        printf(VERT "%s a ajouté %s dans le groupe %s.\n" RESET, clients[emetteur].nom_utilisateur, donnees, nom_groupe);
                        message msgNotif;
                        strncpy(msgNotif.donnees, nom_groupe, 20);
                        msgNotif.type = AJOUT_MEMBRE;
                        if(send(clients[number_client].socket, &msgNotif, sizeof(msgNotif), 0) < 0)
                        {
                            perror("Erreur d'envoi d'un message de la liste d'utilisateurs.");
                            exit(1);
                        }
                    }
                }
            }
        }
    }

    if(send(clients[emetteur].socket, &msg, sizeof(msg), 0) < 0)
    {
        perror("Erreur d'envoi d'un message de la liste d'utilisateurs.");
        exit(1);
    }
}

/***********************************************/
// Fonction permetant d'envoyer un message a un groupe
/***********************************************/
void envoi_message_groupe(info_connexion clients[], info_groupe groupes[], int emetteur, char *nom_groupe, char *donnees){
    int i = 0;
    bool trouve = false;
    int number_groupe = 0;
    while(!trouve && i < MAX_GROUPES){
        if(groupes[i].nombre_membres>0){
            if(strcmp(groupes[i].nom_groupe, nom_groupe) == 0 ){
                trouve = true;
                number_groupe = i;
            }
        }
        ++i;
    }

    if(!trouve){
        printf(ROUGE "Un utilisateur a tenté d'envoyer un message à un groupe inexistant.\n" RESET);
        //envoyer groupe inexistant !!
    }else{
        if(appartient_groupe(clients[emetteur],groupes[number_groupe])){
            for(i=0; i<groupes[number_groupe].nombre_membres; ++i){ 
               if(strcmp(groupes[number_groupe].membres[i].nom_utilisateur,clients[emetteur].nom_utilisateur)!=0){
                    message msg;
                    msg.type = MESSAGE_GROUPE;
                    strncpy(msg.nom_utilisateur, nom_groupe, 44);
                    char str[] ="-->";
                    strncat(msg.nom_utilisateur, str ,strlen(str));
                    strncat(msg.nom_utilisateur, clients[emetteur].nom_utilisateur, strlen(clients[emetteur].nom_utilisateur));
                    strncpy(msg.donnees, donnees, 256);

                    if(send(groupes[number_groupe].membres[i].socket, &msg, sizeof(msg), 0) < 0)
                    {
                        printf("%d ???", groupes[number_groupe].nombre_membres);
                        perror("Erreur d'envoi d'un message de groupe.");
                    }
                }
            }
        }else{
            //envoyer impossible d'envoyer un message à ce groupe!!
        }
    }
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
void *analyse_message_utilisateur(void *arg)
{
    data* donnees;
    donnees = (data*) arg;
    int read_size;
    message msg;
    
    info_connexion* clients = donnees->clients;
    info_groupe* groupes = donnees->groupes;
    int emetteur = donnees->emetteur;
    int* clientstest = donnees->clientstest;
    if(donnees->clientstest[emetteur]==1){
        if((read_size = recv(clients[emetteur].socket, &msg, sizeof(message), 0)) == 0)
        {
            if(!(strcmp(clients[emetteur].nom_utilisateur,"")==0)){
                close(clients[emetteur].socket);
                synchronise_groupes(clients[emetteur],groupes);
                clients[emetteur].socket = 0;

                char nom_utilisateur[20];
                strncpy(nom_utilisateur, clients[emetteur].nom_utilisateur, 20);

                if(!(strcmp(nom_utilisateur,"")==0)){
                    printf(ROUGE "Un utilisateur s'est déconnecté: %s.\n" BLANC, nom_utilisateur);
                    envoi_message_deconnexion(clients, nom_utilisateur);
                    strncpy(clients[emetteur].nom_utilisateur,"",20);
                }
            }
            clientstest[emetteur]=0;
        } else {
            switch(msg.type)
            {
                case UTILISATEURS:
                    envoi_liste_utilisateurs(clients, emetteur);
                break;

                case UTILISATEUR: ;
                    int i;
                    bool ok = true;
                    for(i = 0; i < MAX_CLIENTS; i++)
                    {
                        if(clients[i].socket != 0 && strcmp(clients[i].nom_utilisateur, msg.nom_utilisateur) == 0)
                        {
                            message msg;
                            msg.type =  NON_VALIDE;
                            if(send(clients[emetteur].socket, &msg, sizeof(msg), 0) < 0)
                            {
                                perror("Erreur d'envoi d'un message connexion.");
                                exit(1);
                            }
                            close(clients[emetteur].socket);
                            clients[emetteur].socket = 0;
                            ok = false;
                            clientstest[emetteur]=0;
                            pthread_exit(NULL);
                        }
                    }
                    if(ok){
                        strcpy(clients[emetteur].nom_utilisateur, msg.nom_utilisateur);
                        printf(VERT "Un utilisateur s'est connecté: %s\n" RESET, clients[emetteur].nom_utilisateur);
                        envoi_message_connexion(clients, emetteur);
                    }
                break;

                case MESSAGE_PUBLIC:
                    envoi_message_public(clients, emetteur, msg.donnees);
                    printf(BLEU "Un message public a été reçu puis distribué à tous les utilisateurs.\n" RESET);
                break;

                case MESSAGE_PRIVEE:
                    envoi_message_privee(clients, emetteur, msg.nom_utilisateur, msg.donnees);
                    printf(CYAN "Un message privée a été reçu puis envoyé au destinataire.\n" RESET);
                break;

                case CREER_GOUPE:
                    creer_groupe(clients, groupes, emetteur, msg.nom_utilisateur);
                break;

                case AJOUT_MEMBRE:
                    ajout_membre_groupe(clients, groupes, emetteur, msg.nom_utilisateur, msg.donnees);
                break;

                case MESSAGE_GROUPE:
                    envoi_message_groupe(clients, groupes, emetteur, msg.nom_utilisateur, msg.donnees);
                    printf(CYAN "Un message pour le groupe : %s a été reçu puis envoyé aux membres de ce groupe.\n" RESET, msg.nom_utilisateur);

                break;

                default:
                    // fprintf(stderr, "Le message reçu est inconnu.\n"); // Mis en commentaire suite aux threads
                break;
            }
            clientstest[emetteur]=0;
        }
    }
    pthread_exit(NULL);
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
    info_groupe groupes[MAX_GROUPES];
    data donnees;
    int clientstest[MAX_CLIENTS];

    int i;
    for(i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].socket = 0;
        strncpy(clients[i].nom_utilisateur,"",20);
        clientstest[i] = 0;
    }

    for(i = 0; i < MAX_GROUPES; i++){
        groupes[i].nombre_membres = 0;
    }

    donnees.groupes = groupes;
    donnees.clients = clients;
    donnees.clientstest = clientstest;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    initialisation_du_serveur(&informationServeur, atoi(argv[1]));

    while(true)
    {
        sleep(1);
        int max_fd = construct_fd_set(&file_descriptors, &informationServeur, clients);
        //Problème venant d'ici suite aux threads
        // sleep(0.5);
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
                donnees.emetteur = i;
                if(donnees.clientstest[i]==0){
                    donnees.clientstest[i]=1;
                    pthread_t thread1;

                    if(pthread_create(&thread1, NULL, analyse_message_utilisateur,  (void *)&donnees) == -1) {
                            perror("pthread_create");
                            return EXIT_FAILURE;
                    }
                }
            }
        }
    }

    return 0;
}
