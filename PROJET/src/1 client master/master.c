// master.c — version minimale, FIFO ↔ FIFO

#include <fcntl.h>     // open
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>  // mkfifo
#include <unistd.h>    // read, write, close, unlink

#include "master_client.h"

/******************************
 * Fonction utilitaire: erreur
 ******************************/
static void usage(const char *exe, const char *msg) {
    fprintf(stderr, "usage : %s (sans argument)\n", exe);
    if (msg) fprintf(stderr, "message : %s\n", msg);
    exit(EXIT_FAILURE);
}

/******************************
 * Boucle principale du master
 ******************************/
void loop() {
    while (1) {
        printf("[MASTER] Attente d'un client...\n");

        /* --- Lecture depuis le client --- */
        int fdIn = open(FIFO_CLIENT_TO_MASTER, O_RDONLY);
        int order = ORDER_NONE;
        int number = 0;

        read(fdIn, &order, sizeof(order));

        if (order == ORDER_COMPUTE_PRIME) {
            read(fdIn, &number, sizeof(number));
            printf("[MASTER] Reçu COMPUTE %d\n", number);
        } else if (order == ORDER_STOP) {
            printf("[MASTER] Reçu STOP\n");
        } else {
            printf("[MASTER] Reçu ordre inconnu (%d)\n", order);
        }

        close(fdIn);

        /* --- Calcul simple pour la démo --- */
        int resultat = 0;

        if (order == ORDER_COMPUTE_PRIME) {
            resultat = number * 2;         // résultat bidon
            printf("[MASTER] Envoi résultat %d\n", resultat);
        } else if (order == ORDER_STOP) {
            resultat = 0;                  // ACK
            printf("[MASTER] Envoi ACK STOP\n");
        } else {
            resultat = -42;                // code erreur
        }

        /* --- Envoi vers le client --- */
        int fdOut = open(FIFO_MASTER_TO_CLIENT, O_WRONLY);
        write(fdOut, &resultat, sizeof(resultat));
        close(fdOut);

        if (order == ORDER_STOP)
            break;
    }
}

/******************************
 * Programme principal
 ******************************/
int main(int argc, char *argv[]) {
    if (argc != 1)
        usage(argv[0], "Le master ne prend pas d'arguments");

    printf("[MASTER] Démarrage du master (version minimale)\n");

    mkfifo(FIFO_CLIENT_TO_MASTER, 0666);
    mkfifo(FIFO_MASTER_TO_CLIENT, 0666);

    loop();

    unlink(FIFO_CLIENT_TO_MASTER);
    unlink(FIFO_MASTER_TO_CLIENT);

    printf("[MASTER] Fermeture.\n");
    return EXIT_SUCCESS;
}
