// client.c — version minimale FIFO + sémaphores (mutex + sync)

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>

#include "master_client.h"

#define TK_STOP "stop"
#define TK_COMPUTE "compute"
#define TK_HOW_MANY "howmany"
#define TK_HIGHEST "highest"
#define TK_LOCAL "local"

static void usage(const char *exeName, const char *message) {
  fprintf(stderr, "usage : %s <ordre> [<nombre>]\n", exeName);
  fprintf(stderr, "   ordre \"" TK_STOP "\" : arrêt master\n");
  fprintf(stderr, "   ordre \"" TK_COMPUTE "\" : calcul de nombre premier\n");
  fprintf(stderr, "                       <nombre> doit être fourni\n");
  fprintf(stderr, "   ordre \"" TK_HOW_MANY
                  "\" : combien de nombres premiers calculés\n");
  fprintf(stderr, "   ordre \"" TK_HIGHEST
                  "\" : quel est le plus grand nombre premier calculé\n");
  fprintf(stderr,
          "   ordre \"" TK_LOCAL "\" : calcul de nombres premiers en local\n");
  if (message != NULL) fprintf(stderr, "message : %s\n", message);
  exit(EXIT_FAILURE);
}

static int parseArgs(int argc, char *argv[], int *number) {
  int order = ORDER_NONE;

  if ((argc != 2) && (argc != 3))
    usage(argv[0], "Nombre d'arguments incorrect");

  if (strcmp(argv[1], TK_STOP) == 0)
    order = ORDER_STOP;
  else if (strcmp(argv[1], TK_COMPUTE) == 0)
    order = ORDER_COMPUTE_PRIME;
  else if (strcmp(argv[1], TK_HOW_MANY) == 0)
    order = ORDER_HOW_MANY_PRIME;
  else if (strcmp(argv[1], TK_HIGHEST) == 0)
    order = ORDER_HIGHEST_PRIME;
  else if (strcmp(argv[1], TK_LOCAL) == 0)
    order = ORDER_COMPUTE_PRIME_LOCAL;

  if (order == ORDER_NONE) usage(argv[0], "ordre incorrect");
  if ((order == ORDER_STOP) && (argc != 2))
    usage(argv[0], TK_STOP " : il ne faut pas de second argument");
  if ((order == ORDER_COMPUTE_PRIME) && (argc != 3))
    usage(argv[0], TK_COMPUTE " : il faut le second argument");
  if ((order == ORDER_HOW_MANY_PRIME) && (argc != 2))
    usage(argv[0], TK_HOW_MANY " : il ne faut pas de second argument");
  if ((order == ORDER_HIGHEST_PRIME) && (argc != 2))
    usage(argv[0], TK_HIGHEST " : il ne faut pas de second argument");
  if ((order == ORDER_COMPUTE_PRIME_LOCAL) && (argc != 3))
    usage(argv[0], TK_LOCAL " : il faut le second argument");
  if ((order == ORDER_COMPUTE_PRIME) || (order == ORDER_COMPUTE_PRIME_LOCAL)) {
    *number = strtol(argv[2], NULL, 10);
    if (*number < 2) usage(argv[0], "le nombre doit être >= 2");
  }

  return order;
}

int main(int argc, char *argv[]) {
  /* ------------------------------------------------------------------
   * 1) RÉCUPÉRATION DES DEUX SÉMAPHORES CRÉÉS PAR LE MASTER
   * ------------------------------------------------------------------
   *
   * ftok("master.c", 'M') :
   *   - génère une clé unique basée sur le fichier master.c + identifiant 'M'
   *   - cette clé doit être LA MÊME dans le master et le client
   *   - sinon ils n'accèdent pas au même sémaphore - projet cassé
   *
   * sem_mutex :
   *   - sémaphore d’exclusion mutuelle (mutex)
   *   - empêche deux clients de parler au master en même temps
   *
   * sem_sync :
   *   - sémaphore de synchronisation (sync)
   *   - permet au master d’attendre que le client ait fini de lire la réponse
   *     avant de passer au client suivant
   */
  int key_mutex = ftok("master.c", 'M');  // clé du mutex
  int key_sync = ftok("master.c", 'S');   // clé du sémaphore sync

  int sem_mutex =
      semget(key_mutex, 1, 0666);  // récupère le mutex créé par le master
  int sem_sync =
      semget(key_sync, 1, 0666);  // récupère le sync créé par le master

  // on vérifie que les semaphores existent (normalement créés par master.c)
  assert(sem_mutex != -1);
  assert(sem_sync != -1);

  /* ------------------------------------------------------------------
   * 2) ANALYSE DES ARGUMENTS
   *    - permet de savoir si on fait COMPUTE, STOP, HOWMANY, HIGHEST, ...
   * ------------------------------------------------------------------ */
  int number = 0;
  int order = parseArgs(argc, argv, &number);

  /* ------------------------------------------------------------------
   * 3) CAS PARTICULIER : calcul local
   * ------------------------------------------------------------------
   *
   * ORDER_COMPUTE_PRIME_LOCAL signifie :
   *   - le client doit calculer la primalité lui-même
   *   - PAS de communication avec le master
   *   - PAS de sémaphore
   */
  if (order == ORDER_COMPUTE_PRIME_LOCAL) {
    printf("[CLIENT] Local mode (pas de master)\n");
    return 0;
  }

  /* ------------------------------------------------------------------
   * 4) SECTION CRITIQUE - MUTEX
   * ------------------------------------------------------------------
   *
   * On prend le mutex :
   *   - si un autre client est déjà en communication avec le master,
   *     celui-là attend ici (bloqué dans P)
   *   - sinon il entre dans la SECTION CRITIQUE
   *
   * L’objectif : NE JAMAIS avoir 2 clients en même temps dans :
   *        write() vers FIFO_CLIENT_TO_MASTER
   *        read()  depuis FIFO_MASTER_TO_CLIENT
   * ------------------------------------------------------------------ */
  P(sem_mutex);

  /* ------------------------------------------------------------------
   * 5) ENVOI DU MESSAGE AU MASTER
   * ------------------------------------------------------------------
   *
   * On ouvre la FIFO d’écriture - client - master
   * On envoie d’abord l’order (STOP, COMPUTE, …)
   * Puis éventuellement le nombre si COMPUTE
   */
  int fdOut = open(FIFO_CLIENT_TO_MASTER, O_WRONLY);
  write(fdOut, &order, sizeof(order));

  if (order == ORDER_COMPUTE_PRIME) write(fdOut, &number, sizeof(number));

  close(fdOut);  // fin de l’envoi

  /* ------------------------------------------------------------------
   * 6) LECTURE DE LA RÉPONSE DU MASTER
   * ------------------------------------------------------------------
   *
   * Le client ouvre la FIFO master - client
   * Le master y a écrit un int = résultat
   */
  int fdIn = open(FIFO_MASTER_TO_CLIENT, O_RDONLY);

  int resultat;
  read(fdIn, &resultat, sizeof(resultat));
  close(fdIn);

  /* ------------------------------------------------------------------
   * 7) FIN DE LA SECTION CRITIQUE
   * ------------------------------------------------------------------
   *
   * V(sem_mutex) :
   *      - on libère le verrou
   *      - un autre client peut entrer en section critique
   *
   * V(sem_sync) :
   *      - signal envoyé AU MASTER
   *      - "j'ai fini de lire la réponse, tu peux passer au client suivant"
   *
   * C’est exactement ce que veut le sujet CC2 :
   *   - Un mutex pour protéger la communication
   *   - Un sémaphore sync pour orchestrer master-client
   * ------------------------------------------------------------------ */
  V(sem_mutex);  // permet à un autre client d’entrer

  V(sem_sync);  // indique au master : "communication FINIE !"

  /* ------------------------------------------------------------------
   * 8) INTERPRÉTATION DU RÉSULTAT
   * ------------------------------------------------------------------
   *
   * clientInterpretOrder() se charge d’afficher :
   *   - premier / pas premier
   *   - how many
   *   - highest
   *   - stop
   */
  clientInterpretOrder(order, number, resultat);

  return 0;
}
