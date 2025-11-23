// client.c — version minimale : juste communication avec le master via FIFOs

#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>  // open
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

/************************************************************************
 * Fonction principale : JUSTE communication client -> master via FIFOs
 ************************************************************************/

int main(int argc, char *argv[]) {
  int number = 0;
  int order = parseArgs(argc, argv, &number);

  // Cas particulier : mode local
  if (order == ORDER_COMPUTE_PRIME_LOCAL) {
    printf("[CLIENT] Mode local pas implémenté (aucune comm. master).\n");
    return EXIT_SUCCESS;
  }

  // --- Envoi de la requête au master ---
  int fdOut = open(FIFO_CLIENT_TO_MASTER, O_WRONLY);

  // On laisse clientSendOrder gérer l'envoi (pas besoin d'écrire order ici)
  clientSendOrder(fdOut, order, number);

  close(fdOut);

  // --- Réception de la réponse ---
  int fdIn = open(FIFO_MASTER_TO_CLIENT, O_RDONLY);

  int resultat;
  read(fdIn, &resultat, sizeof(resultat));
  close(fdIn);

  // Interprétation via ta fonction utilitaire
  clientInterpretOrder(order, number, resultat);

  return EXIT_SUCCESS;
}
