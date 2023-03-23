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


int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        printf("Error: Expected 1 parameter, got %d\n", argc);
        exit(1);
    }

    if (strcmp(argv[1], "exit") == 0)
    {
        exit(0);
    }
    
    //ESCREVER PARTE argv[1] == "stats" PARA LER OS STATS DOS SENSORES


    //ESCREVER PARTE argv[1] == "reset" PARA LIMPAR ESTATISTICAS


    //ESCREVER PARTE argv[1] == "sensors" PARA MOSTRAR TODOS OS SENSORES Q JA ENVIARAM INFO PARA O SISTEMA


    //if first argument = 'add_alert', expect 4 more arguments
    if (strcmp(argv[1], "add_alert") == 0)
    {
        if (argc != 6)
        {
            printf("Error: Expected 5 parameters, got %d\n", argc);
            exit(1);
        }
        if (strlen(argv[2]) < 3 || strlen(argv[2]) > 32)
        {
            printf("Error: Expected second parameter to be between 3 and 32 characters, got %d\n", strlen(argv[2]));
            exit(1);
        }
        for (int i = 0; i < strlen(argv[2]); i++)
        {
            if (argv[2][i] != '_' && !isalnum(argv[2][i]))
            {
                printf("Error: Expected second parameter to be a string with characters, numbers, and _, got %c\n", argv[2][i]);
                exit(1);
            }
        }
       
        if (atoi(argv[4]) > atoi(argv[5]))
        {
            printf("Error: Expected fourth parameter to be <= fifth parameter, got %d and %d\n", atoi(argv[4]), atoi(argv[5]));
            exit(1);
        }

        int id = atoi(argv[2]);
        char *key = argv[3];
        int min = atoi(argv[4]);
        int max = atoi(argv[5]);


    }






    return 0;
}