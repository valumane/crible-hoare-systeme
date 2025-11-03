#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "myassert.h"

#include "master_client.h"
#include "master_worker.h"

/************************************************************************
 * Données persistantes d'un master
 ************************************************************************/

// on peut ici définir une structure stockant tout ce dont le master
// a besoin

    // nom des tubes nommés
    const char *fifoClientToMaster = "client_to_master.fifo";
    const char *fifoMasterToClient = "master_to_client.fifo";


/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/

static void usage(const char *exeName, const char *message)
{
    fprintf(stderr, "usage : %s\n", exeName);
    if (message != NULL)
        fprintf(stderr, "message : %s\n", message);
    exit(EXIT_FAILURE);
}


/************************************************************************
 * boucle principale de communication avec le client
 ************************************************************************/
void loop(/* paramètres */)
{
    // boucle infinie :
    // - ouverture des tubes (cf. rq client.c)
    // - attente d'un ordre du client (via le tube nommé)
    // - si ORDER_STOP
    //       . envoyer ordre de fin au premier worker et attendre sa fin
    //       . envoyer un accusé de réception au client
    // - si ORDER_COMPUTE_PRIME
    //       . récupérer le nombre N à tester provenant du client
    //       . construire le pipeline jusqu'au nombre N-1 (si non encore fait) :
    //             il faut connaître le plus grand nombre (M) déjà enovoyé aux workers
    //             on leur envoie tous les nombres entre M+1 et N-1
    //             note : chaque envoie déclenche une réponse des workers
    //       . envoyer N dans le pipeline
    //       . récupérer la réponse
    //       . la transmettre au client
    // - si ORDER_HOW_MANY_PRIME
    //       . transmettre la réponse au client (le plus simple est que cette
    //         information soit stockée en local dans le master)
    // - si ORDER_HIGHEST_PRIME
    //       . transmettre la réponse au client (le plus simple est que cette
    //         information soit stockée en local dans le master)
    // - fermer les tubes nommés
    // - attendre ordre du client avant de continuer (sémaphore : précédence)
    // - revenir en début de boucle
    //
    // il est important d'ouvrir et fermer les tubes nommés à chaque itération
    // voyez-vous pourquoi ?
}


/************************************************************************
 * Fonction principale
 ************************************************************************/

int main(int argc, char * argv[])
{
    if (argc != 1)
        usage(argv[0], NULL);

    // - création des sémaphores
        // ipc ( mutex_clients, sync_client )

    // - création des tubes nommés
        // client_to_master.fifo : recevoir les demandes du client
        printf("Création du tube nommé client_to_master.fifo\n");
        mkfifo(fifoClientToMaster, 0666);

        fopen("client_to_master.fifo", "r"); // ouverture en lecture seule pour éviter EOF immédiat
        

        
/*
        // master_to_client.fifo : pour envoyer les resultats au client
        printf("Création du tube nommé master_to_client.fifo\n");
        mkfifo(fifoMasterToClient, 0666);
*/
        

    // - création du premier worker
        // creation de deux pipes anonyme :
            // pipe(to_worker) : pour envoyer des ordres au worker
            

            // pipe(from_worker) : pour recevoir des réponses du worker

        // fork()
        // dans le fils :
            // execv( "worker", {"worker", "2", fdIn, fdToMaster, NULL} )
        // dans le père :
            // to_worker[1] : pour ecrire vers worker
            // from_worker[0] : pour lire worker / pipeline
            // ferme les extremités inutilisées des pipes

    // boucle infinie
    loop(/* paramètres */);

    // destruction des tubes nommés, des sémaphores, ...
        //unlink(fifoClientToMaster);
        //unlink(fifoMasterToClient);

    return EXIT_SUCCESS;
}

// N'hésitez pas à faire des fonctions annexes ; si les fonctions main
// et loop pouvaient être "courtes", ce serait bien
