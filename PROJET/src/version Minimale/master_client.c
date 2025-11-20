#if defined HAVE_CONFIG_H
#include "config.h"
#endif

#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>

#include "myassert.h"

#include "master_client.h"


#include <sys/stat.h>   // <-- pour mkfifo()
#include <unistd.h>     // <-- pour close() et unlink()
#include <errno.h>      // <-- pour errno si tu veux gérer les erreurs



// fonctions éventuelles internes au fichier
void createFifos(){
  mkfifo(FIFO_CLIENT_TO_MASTER, 0666);
  mkfifo(FIFO_MASTER_TO_CLIENT, 0666);
}

void closePipes(int pipe1, int pipe2){
  close(pipe1);
  close(pipe2);
}

void unlinkPipes(){
  unlink(FIFO_CLIENT_TO_MASTER);  // supprimer la FIFO client->master
  unlink(FIFO_MASTER_TO_CLIENT);  // supprimer la FIFO master->client
}

// fonctions éventuelles proposées dans le .h

