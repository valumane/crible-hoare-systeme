#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>

const char *fifoClientToMaster = "client_to_master.fifo";
const char *fifoMasterToClient = "master_to_client.fifo";
int res;


// fonction de création du tube nommé et 
int createTube(const char *fifoName, int flags){
    int fd = mkfifo(fifoName, flags);
    assert(fd != -1);
    return fd;
}

int main(void){
    // création du tube nommé
    printf("Création du tube nommé client_to_master.fifo\n");
    createTube(fifoClientToMaster, 0666);

    // ouverture bloquante : attend qu’un client écrive
    int fdRead = open(fifoClientToMaster, O_RDONLY); //j'ouvre le tube en lecture ( READ ONLY )
    assert(fdRead != -1); //je test si le tube est bien ouvert
    fprintf(stdout,"Tube ouvert, en attente d’un nombre...\n"); 

    int nombre;
    int n = read(fdRead, &nombre, sizeof(int)); // je lis le nombre envoyé par le client


    // affichage du nombre reçu
    if (n == sizeof(int)) {  // si j'ai bien lu un int
        fprintf(stdout,"Nombre reçu du client : %d\n", nombre);
    } else if (n == 0) { // le client a fermé le tube
        fprintf(stdout,"Le client a fermé le tube sans rien envoyer.\n");
    } else { // erreur de lecture
        fprintf(stderr,"Erreur de lecture");
    }

    // fermeture des tubes
    close(fdRead);
    // suppression des tubes nommés
    unlink(fifoClientToMaster); 

    return EXIT_SUCCESS;
}