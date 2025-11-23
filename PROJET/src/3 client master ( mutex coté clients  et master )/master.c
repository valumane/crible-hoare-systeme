// master.c — version minimale FIFO + sémaphores (mutex + sync)

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <unistd.h>

#include "master_client.h"

/* ------------------------------------------------------------------
 * Fonction d'affichage d'erreur : le master ne prend AUCUN argument.
 * ------------------------------------------------------------------ */
static void usage(const char *exe, const char *msg) {
  fprintf(stderr, "usage : %s\n", exe);
  if (msg) fprintf(stderr, "message : %s\n", msg);
  exit(EXIT_FAILURE);
}

/* ==================================================================
 *  Boucle principale du master (communication FIFO uniquement)
 * ==================================================================
 *
 *  sem_sync = sémaphore de synchronisation :
 *
 *      - master :  P(sem_sync)  - attend que le client dise "j'ai fini"
 *      - client :  V(sem_sync)  - indique : "j'ai tout lu, tu peux continuer"
 *
 *  BUT :
 *      Empêcher le master d’accepter un nouveau client tant que
 *      le client précédent n’a pas fini de lire sa réponse.
 *
 * ================================================================== */
void loop(int sem_sync) {
  while (1) {
    printf("[MASTER] Attente d'un client...\n");

    /* --------------------------------------------------------------
     * 1) OUVERTURE DE LA FIFO client - master
     * --------------------------------------------------------------
     * On attend qu’un client envoie un ordre.
     *
     * open() est bloquant tant qu’aucun client n'ouvre en écriture.
     */
    int fdIn = open(FIFO_CLIENT_TO_MASTER, O_RDONLY);

    int order = ORDER_NONE;
    int number = 0;

    /* --------------------------------------------------------------
     * 2) LECTURE DE L’ORDRE ENVOYÉ PAR LE CLIENT
     * -------------------------------------------------------------- */
    read(fdIn, &order, sizeof(order));

    /* --------------------------------------------------------------
     * 3) SI L’ORDRE EST COMPUTE, ON LIT LE NOMBRE QUI SUIT
     * -------------------------------------------------------------- */
    if (order == ORDER_COMPUTE_PRIME) {
      read(fdIn, &number, sizeof(number));
      printf("[MASTER] Reçu COMPUTE %d\n", number);
    } else if (order == ORDER_STOP) {
      printf("[MASTER] Reçu STOP\n");
    }

    close(fdIn);  // Fin de la réception

    /* --------------------------------------------------------------
     * 4) TRAITEMENT DE L’ORDRE (VERSION MINIMALISTE)
     * --------------------------------------------------------------
     * Ici le master ne calcule pas la primalité.
     * - il renvoie juste number * 2 pour la démo.
     *
     * Dans la vraie version, ce sera le pipeline Hoare qui répondra.
     */
    int resultat = 0;

    if (order == ORDER_COMPUTE_PRIME) resultat = number * 2;

    /* --------------------------------------------------------------
     * 5) ENVOI DE LA RÉPONSE AU CLIENT
     * --------------------------------------------------------------
     * Le client lira ce "resultat" dans sa FIFO.
     */
    int fdOut = open(FIFO_MASTER_TO_CLIENT, O_WRONLY);
    write(fdOut, &resultat, sizeof(resultat));
    close(fdOut);

    /* --------------------------------------------------------------
     * 6) SYNCHRONISATION : MASTER DOIT ATTENDRE LE CLIENT
     * --------------------------------------------------------------
     * Le client fait V(sem_sync) après avoir lu la réponse.
     *
     * Ici, le master fait P(sem_sync) :
     *      - il attend que le client FINISSE AVANT de traiter un
     *        autre client.
     *
     * Cela évite que deux clients lisent/écrivent dans les FIFOs
     * en même temps.
     */
    P(sem_sync);

    /* --------------------------------------------------------------
     * 7) SI L’ORDRE EST STOP, ON ARRÊTE LE MASTER
     * -------------------------------------------------------------- */
    if (order == ORDER_STOP) break;
  }
}

/* ==================================================================
 *                        FONCTION PRINCIPALE
 * ==================================================================
 *
 * Globalement :
 *
 * 1) Crée les FIFO
 * 2) Crée les deux sémaphores :
 *        - mutex : exclusion mutuelle entre les clients
 *        - sync  : synchro master - client
 *
 * 3) Lance la boucle principale
 * 4) Nettoie les ressources IPC
 *
 * ================================================================== */
int main(int argc, char *argv[]) {
  if (argc != 1) usage(argv[0], "pas d'argument attendu");

  printf("[MASTER] Démarrage master (mutex + sync)\n");

  /* --------------------------------------------------------------
   * 1) CRÉATION DES FIFO
   * -------------------------------------------------------------- */
  createFifos();

  /* --------------------------------------------------------------
   * 2) CRÉATION DES DEUX SÉMAPHORES
   * --------------------------------------------------------------
   *
   * ftok("master.c", 'M') :
   *      - clé pour le sémaphore mutex
   *
   * ftok("master.c", 'S') :
   *      - clé pour le sémaphore sync
   *
   * semget(..., IPC_CREAT, ...) :
   *      - crée le sémaphore si pas encore existant
   *      - sinon l’ouvre
   *
   * semctl(... SETVAL ...) :
   *      - initialise la valeur :
   *
   *      mutex = 1 (libre)
   *      sync  = 0 (master DOIT attendre)
   * -------------------------------------------------------------- */
  int key_mutex = ftok("master.c", 'M');
  int key_sync = ftok("master.c", 'S');

  int sem_mutex = semget(key_mutex, 1, IPC_CREAT | 0666);
  int sem_sync = semget(key_sync, 1, IPC_CREAT | 0666);

  semctl(sem_mutex, 0, SETVAL, 1);  // mutex = 1 - un client peut entrer
  semctl(sem_sync, 0, SETVAL, 0);   // sync = 0  - master attend

  /* --------------------------------------------------------------
   * 3) BOUCLE PRINCIPALE
   * -------------------------------------------------------------- */
  loop(sem_sync);

  /* --------------------------------------------------------------
   * 4) NETTOYAGE (FIFO + sémaphores)
   * -------------------------------------------------------------- */
  unlinkPipes();

  semctl(sem_mutex, 0, IPC_RMID);
  semctl(sem_sync, 0, IPC_RMID);

  printf("[MASTER] Fermeture.\n");
  return 0;
}
