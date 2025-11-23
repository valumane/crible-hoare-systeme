// master.c — version minimale FIFO + création du mutex

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <unistd.h>

#include "master_client.h"

static void usage(const char *exe, const char *msg) {
    fprintf(stderr, "usage : %s\n", exe);
    if (msg) fprintf(stderr, "message : %s\n", msg);
    exit(EXIT_FAILURE);
}

void loop() {
    while (1) {
        printf("[MASTER] Attente d'un client...\n");

        int fdIn = open(FIFO_CLIENT_TO_MASTER, O_RDONLY);

        int order = 0;
        int number = 0;

        read(fdIn, &order, sizeof(order));

        if (order == ORDER_COMPUTE_PRIME) {
            read(fdIn, &number, sizeof(number));
            printf("[MASTER] Reçu COMPUTE %d\n", number);
        } else if (order == ORDER_STOP) {
            printf("[MASTER] Reçu STOP\n");
        }

        close(fdIn);

        int resultat = 0;
        if (order == ORDER_COMPUTE_PRIME) resultat = number * 2;

        int fdOut = open(FIFO_MASTER_TO_CLIENT, O_WRONLY);
        write(fdOut, &resultat, sizeof(resultat));
        close(fdOut);

        if (order == ORDER_STOP) break;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 1) usage(argv[0], "pas d’argument attendu");

    printf("[MASTER] Démarrage master (minimal + mutex)\n");

    createFifos();

    key_t key_mutex = ftok("master.c", 'M');
    int sem_mutex = semget(key_mutex, 1, IPC_CREAT | 0666);

    // mutex = 1
    semctl(sem_mutex, 0, SETVAL, 1);

    loop();
    
    unlinkPipes();

    semctl(sem_mutex, 0, IPC_RMID);

    printf("[MASTER] Fermeture.\n");
    return 0;
}
