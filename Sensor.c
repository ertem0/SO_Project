#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>  
#include <stdarg.h>
#include "SharedMEM.h"
#include <string.h>
#include <ctype.h>


//generate random munber between min and max parameters
int random_number(int min, int max)
{
    return rand() % (max - min + 1) + min;
}



int main(int argc, char *argv [])
{

    if (argc != 6)
    {
        printf("Error: Expected 5 parameters, got %d\n", argc);
        exit(1);
    }
    //id parameter (SENS1) size must be between 3 and 32
    if (strlen(argv[1]) < 3 || strlen(argv[1]) > 32)
    {
        printf("Error: Expected first parameter to be between 3 and 32 characters, got %ld\n", strlen(argv[1]));
        exit(1);
    }
    if (atoi(argv[2]) < 0 ){
        printf("Error: Expected second parameter to be a positive number, got %d\n", atoi(argv[2]));
        exit(1);
    }
    //forth parameter can be a string with characters, numbers, and _
    for (size_t i = 0; i < strlen(argv[3]); i++)
    {
        if (argv[3][i] != '_' && !isalnum(argv[3][i]))
        {
            printf("Error: Expected third parameter to be a string with characters, numbers, and _, got %c\n", argv[3][i]);
            exit(1);
        }
    }
    char *id = argv[1];
    char *key = argv[3];
    int value = random_number(atoi(argv[4]), atoi(argv[5]));
    
    //SEND TO NAMED PIPE
    
    // char *fifo = "/tmp/pipe";
    // mkfifo(fifo, 0666);
    // FILE *fp;
    // fp = fopen(fifo, "w");
    // fprintf(fp, "%d#%s#%d", id, key, value);
    // fclose(fp);

    //print id, key and value
    printf("%s\n%s\n%d\n", id, key, value);





    return 0;
}


