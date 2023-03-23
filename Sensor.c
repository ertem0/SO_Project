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

    if (argc != 5)
    {
        printf("Error: Expected 4 parameters, got %d\n", argc);
        exit(1);
    }
    if (atoi(argv[1]) < 0)
    {
        printf("Error: Expected second parameter to be => 0, got %d\n", atoi(argv[1]));
        exit(1);
    }
    //first parameter size must be between 3 and 32
    if (strlen(argv[2]) < 3 || strlen(argv[2]) > 32)
    {
        printf("Error: Expected third parameter to be between 3 and 32 characters, got %ld\n", strlen(argv[2]));
        exit(1);
    }
    //first parameter can be a string with characters, numbers, and _
    for (size_t i = 0; i < strlen(argv[2]); i++)
    {
        if (argv[2][i] != '_' && !isalnum(argv[2][i]))
        {
            printf("Error: Expected third parameter to be a string with characters, numbers, and _, got %c\n", argv[2][i]);
            exit(1);
        }
    }
    int id = atoi(argv[1]);
    char *key = argv[2];
    int value = random_number(atoi(argv[3]), atoi(argv[4]));

    //SEND TO NAMED PIPE
    
    // char *fifo = "/tmp/pipe";
    // mkfifo(fifo, 0666);
    // FILE *fp;
    // fp = fopen(fifo, "w");
    // fprintf(fp, "%d#%s#%d", id, key, value);
    // fclose(fp);

    


    return 0;
}


