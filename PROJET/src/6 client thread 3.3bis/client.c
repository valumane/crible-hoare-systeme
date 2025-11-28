#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>    // open
#include <pthread.h>  //thread
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // read, write, close

#include "master_client.h"

// chaînes possibles pour le premier paramètre de la ligne de commande
#define TK_STOP "stop"
#define TK_COMPUTE "compute"
#define TK_HOW_MANY "howmany"
#define TK_HIGHEST "highest"
#define TK_LOCAL "local"

/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/

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
    *number = (int)strtol(argv[2], NULL, 10);
    if (*number < 2) usage(argv[0], "le nombre doit être >= 2");
  }

  return order;
}

typedef struct {
  bool *tab;
  int limit;
  int div;
  pthread_mutex_t *mutex;
} ThreadArgs;

void *threadMark(void *arg) {
  ThreadArgs *a = (ThreadArgs *)arg;
  int d = a->div;
  int N = a->limit;

  for (int k = 2 * d; k <= N; k += d) {
    pthread_mutex_lock(a->mutex);
    a->tab[k] = false;
    pthread_mutex_unlock(a->mutex);
  }

  return NULL;
}


void localmod(int number) {
  // =======================
  // 1) Tableau booléens
  // =======================

  bool *tab = malloc(sizeof(bool) * (number + 1));

  for (int i = 2; i <= number; i++) tab[i] = true;

  // =======================
  // 2) Préparer threads
  // =======================

  pthread_mutex_t mutex;
  pthread_mutex_init(&mutex, NULL);

  int max = mysqrt(number);
  int nbThreads = max - 1;  // pour diviseurs = 2...max

  pthread_t *tids = malloc(sizeof(pthread_t) * nbThreads);
  ThreadArgs *args = malloc(sizeof(ThreadArgs) * nbThreads);

  // =======================
  // 3) Créer les threads
  // =======================

  for (int i = 0; i < nbThreads; i++) {
    args[i].tab = tab;
    args[i].limit = number;
    args[i].div = i + 2;  // diviseur = 2, 3, 4...
    args[i].mutex = &mutex;

    pthread_create(&tids[i], NULL, threadMark, &args[i]);
  }

  // =======================
  // 4) Join des threads
  // =======================

  for (int i = 0; i < nbThreads; i++) pthread_join(tids[i], NULL);

  pthread_mutex_destroy(&mutex);

  // =======================
  // 5) Affichage des nombres premiers
  // =======================

  printf("Premiers trouvés jusqu'à %d :\n", number);
  for (int i = 2; i <= number; i++)
    if (tab[i]) printf("%d ", i);

  printf("\n");

  free(tab);
  free(tids);
  free(args);
}

int main(int argc, char *argv[]) {
  int number = 0;
  int order = parseArgs(argc, argv, &number);

  if (number < 2) {
    fprintf(stderr, "N doit être >= 2\n");
    return EXIT_FAILURE;
  }

  if (order == ORDER_COMPUTE_PRIME_LOCAL) {
    printf("[CLIENT] Mode local : calcul des premiers jusqu'à %d\n", number);
    localmod(number);
    return EXIT_SUCCESS;
  }
  return 0;
}
