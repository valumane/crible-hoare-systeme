#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

const char *fifoClientToMaster = "client_to_master.fifo";
const char *fifoMasterToClient = "master_to_client.fifo";


//ajout fcntl.h pour les opérations sur les fichiers
#include <fcntl.h>
//ajout unistd.h pour les opérations POSIX (read, write, close,
#include <unistd.h>

int main(int argc, char * argv[]){
    assert(argc > 1); //je test si ya bien un argument nombre 
    
    //ouverture du tube nommé en écriture
    int fdWrite = open(fifoClientToMaster, O_WRONLY); //j'ouvre le tube en écriture
    assert(fdWrite != -1); //je test si il a etait ouvert correctement

    //envoi d'un nombre au master
    int nombre = atoi(argv[1]); //argv[1] est un char*, je le converti en int
    write(fdWrite, &nombre, sizeof(int)); //j'ecrit le nombre dans le tube
    fprintf(stdout,"Envoi du nombre %d au master\n", nombre);
    

    //fermeture des tubes
    close(fdWrite);

    return EXIT_SUCCESS;
}
