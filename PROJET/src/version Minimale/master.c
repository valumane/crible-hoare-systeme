#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "master_client.h"
#include "master_worker.h"
#include "myassert.h"

/* include ajouté */
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
/******************/

/************************************************************************
 * Données persistantes d'un master
 ************************************************************************/

// on peut ici définir une structure stockant tout ce dont le master
// a besoin

int last_tested = 2;
int highest_prime = 2;
int nb_primes = 0;  // nombre de nombres premiers trouvés

/************************************************************************
 * Usage et analyse des arguments passés en ligne de commande
 ************************************************************************/

static void usage(const char *exeName, const char *message) {
  fprintf(stderr, "usage : %s\n", exeName);
  if (message != NULL) {
    fprintf(stderr, "message : %s\n", message);
  }
  exit(EXIT_FAILURE);
}

/************************************************************************
 * fonction secondaires *
 ***********************************************************************/
// rend un pipe non bloquant
void set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// order stop
void order_stop(int pipeMW[2]) {
  int stopVal = -1;
  if (write(pipeMW[1], &stopVal, sizeof(stopVal)) != sizeof(stopVal)) {
    perror("[MASTER] write stopVal");
  }

  int fdRetour = open(FIFO_MASTER_TO_CLIENT, O_WRONLY);
  if (fdRetour != -1) {
    int ack = 0;
    write(fdRetour, &ack, sizeof(ack));
    close(fdRetour);
  } else {
    perror("[MASTER] open FIFO_MASTER_TO_CLIENT (STOP)");
  }

  printf("[MASTER] Ordre STOP traité, on sort de la boucle.\n");
}

// order compute
void order_compute(int nombre, int pipeMW[2], int pipeWM[2]) {
  printf(" nbprime : %d\n", nb_primes);

  // Envoi du ou des nombres dans le pipeline
  if (nombre > last_tested) {
    for (int i = last_tested + 1; i <= nombre; i++) {
      printf(" nbprime : %d\n", nb_primes);
      if (write(pipeMW[1], &i, sizeof(i)) != sizeof(i)) {
        printf(" nbprime : %d\n", nb_primes);
        perror("[MASTER] write pipeMW");
        break;
      }
    }
    last_tested = nombre;
  } else {
    if (write(pipeMW[1], &nombre, sizeof(nombre)) != sizeof(nombre)) {
      perror("[MASTER] write pipeMW (nombre)");
      printf(" nbprime : %d\n", nb_primes);
    }
  }

  // Lecture des nouveaux premiers trouvés
  printf(" nbprime : %d\n", nb_primes);
  int prime;
  while (read(pipeWM[0], &prime, sizeof(prime)) > 0) {
    printf(" nbprime : %d\n", nb_primes);
    highest_prime = prime;
    nb_primes++;
    printf("[MASTER] Nouveau nombre premier trouvé : %d\n", prime);
  }

  // Test local (temporaire)
  int isPrime = 1;
  printf(" nbprime : %d\n", nb_primes);
  for (int i = 2; i * i <= nombre; i++) {
    printf(" nbprime : %d\n", nb_primes);
    if (nombre % i == 0) {
      printf(" nbprime : %d\n", nb_primes);
      isPrime = 0;
      break;
    }
  }
  printf(" nbprime : %d\n", nb_primes);

  // Envoi du résultat au client
  int fdRetour = open(FIFO_MASTER_TO_CLIENT, O_WRONLY);
  printf(" nbprime : %d\n", nb_primes);
  if (fdRetour != -1) {
    write(fdRetour, &isPrime, sizeof(isPrime));
    printf(" nbprime : %d\n", nb_primes);
    close(fdRetour);
  } else {
    printf(" nbprime : %d\n", nb_primes);
    perror("[MASTER] open FIFO_MASTER_TO_CLIENT (COMPUTE)");
  }

  printf("[MASTER] Résultat envoyé au client : %d (%s)\n", isPrime,
         isPrime ? "premier" : "non premier");
  printf(" nbprime : %d\n", nb_primes);
}

/************************************************************************
 * boucle principale de communication avec le client
 ************************************************************************/
void loop(int pipeMW[2], int pipeWM[2]) {
  while (1) {  // boucle principale du master
    // === Ouverture de la FIFO côté client ===
    int fdClient = open(FIFO_CLIENT_TO_MASTER, O_RDONLY);
    if (fdClient == -1) {
      perror("[MASTER] open FIFO_CLIENT_TO_MASTER");
      continue;
    }

    // On lit d'abord l'ordre
    int order;
    ssize_t r = read(fdClient, &order, sizeof(order));
    if (r == 0) {
      // rien reçu (client a fermé la FIFO)
      close(fdClient);
      continue;
    }
    if (r != (ssize_t)sizeof(order)) {
      fprintf(stderr, "[MASTER] Ordre incomplet reçu (taille=%zd)\n", r);
      close(fdClient);
      continue;
    }

    int nombre = 0;

    if (order == ORDER_COMPUTE_PRIME) {
      // On doit lire le nombre à tester
      ssize_t rr = read(fdClient, &nombre, sizeof(nombre));
      if (rr != (ssize_t)sizeof(nombre)) {
        fprintf(stderr, "[MASTER] Nombre manquant pour ORDER_COMPUTE_PRIME\n");
        close(fdClient);
        continue;
      }
      printf("[MASTER] Reçu ordre COMPUTE pour %d\n", nombre);
    } else if (order == ORDER_STOP) {
      printf("[MASTER] Reçu ordre STOP\n");
    } else if (order == ORDER_HOW_MANY_PRIME) {
      printf("[MASTER] Reçu ordre HOW_MANY\n");
    } else if (order == ORDER_HIGHEST_PRIME) {
      printf("[MASTER] Reçu ordre HIGHEST\n");
    } else {
      printf("[MASTER] Ordre inconnu : %d\n", order);
      close(fdClient);
      continue;
    }

    close(fdClient);

    // === Traitement des ordres ===

    // 1) STOP : on propage -1 aux workers et on répond au client
    if (order == ORDER_STOP) {
      order_stop(pipeMW);
      break;  // sortir de la boucle principale
    }

    // 2) COMPUTE_PRIME : on garde ton pipeline + test local pour l'instant
    if (order == ORDER_COMPUTE_PRIME) {
      order_compute(nombre, pipeMW, pipeWM);
    }

    /*// 3) HOW_MANY : renvoyer nb_primes
    else if (order == ORDER_HOW_MANY_PRIME) {
      int fdRetour = open(FIFO_MASTER_TO_CLIENT, O_WRONLY);
      if (fdRetour != -1) {
        write(fdRetour, &nb_primes, sizeof(nb_primes));
        close(fdRetour);
      } else {
        perror("[MASTER] open FIFO_MASTER_TO_CLIENT (HOW_MANY)");
      }
    }*/

    // 4) HIGHEST : renvoyer highest_prime
    else if (order == ORDER_HIGHEST_PRIME) {
      int fdRetour = open(FIFO_MASTER_TO_CLIENT, O_WRONLY);
      if (fdRetour != -1) {
        write(fdRetour, &highest_prime, sizeof(highest_prime));
        close(fdRetour);
      } else {
        perror("[MASTER] open FIFO_MASTER_TO_CLIENT (HIGHEST)");
      }
    }
  }
}

/************************************************************************
 * boucle principale de communication avec le client
 ************************************************************************/

int main(void) {
  printf("[MASTER] Démarrage du master\n");

  // création des FIFOs
  createFifos();

  // création des pipes de communication avec le worker
  int pipeMW[2];  // master -> worker
  int pipeWM[2];  // worker -> master
  assert(pipe(pipeMW) == 0);
  assert(pipe(pipeWM) == 0);

  pid_t pid = fork();
  assert(pid != -1);

  if (pid == 0) {  // processus worker
    // fermer les descripteurs inutiles
    closePipes(pipeMW[1],
               pipeWM[0]);  // écriture vers le worker lecture depuis le worker

    // exécuter le worker
    char fdReadStr[10], fdWriteStr[10], primeStr[10];
    snprintf(fdReadStr, sizeof(fdReadStr), "%d",
             pipeMW[0]);  // lecture du master
    snprintf(fdWriteStr, sizeof(fdWriteStr), "%d",
             pipeWM[1]);  // écriture vers le master

    snprintf(primeStr, sizeof(primeStr), "%d", 2);  // premier initial
    char *args[] = {"worker.o", fdReadStr, fdWriteStr, primeStr, NULL};
    execv("./worker.o", args);
    perror("execv");
    exit(EXIT_FAILURE);
  }

  closePipes(pipeMW[0], pipeWM[1]);  // fermer le pipe de lecture vers le worker
                                     // et ecriture depuis le worker
  set_nonblocking(pipeWM[0]);        // lecture non bloquante

  // boucle principale de communication avec le client
  loop(pipeMW, pipeWM);

  // envoi du signal d'arrêt au worker
  closePipes(pipeMW[1],
             pipeWM[0]);  // fermer le pipe d'écriture / lecture vers le worker
  unlinkPipes();           //
  printf("[MASTER] Fermeture propre du master.\n");

  return EXIT_SUCCESS;
}
