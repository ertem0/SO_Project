//TODO: perguntar ao stor q recursos limpar e como sair quando da erro a escrever para o pipe
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#define SENSOR_FIFO "/tmp/sensor_fifo"
int count = 0;
struct sigaction a_sigtstp;
struct sigaction a_sigint;
int sensor_fd;

int random_number(int min, int max)
{
    return rand() % (max - min + 1) + min;
}

void sigtstp_handler()
{
    printf("%d messages sent\n", count);
}

void sigint_handler()
{
    printf("Sensor %d exiting\n", getpid());
    close(sensor_fd);
    exit(0);
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
    //only alfanumeric characters
    for(size_t i = 0; i < strlen(argv[1]); i++)
    {
        if (!isalnum(argv[1][i]))
        {
            printf("Error: Expected first parameter to be a string with characters and numbers, got %c\n", argv[1][i]);
            exit(1);
        }
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
    int time = atoi(argv[2]);
    char *key = argv[3];
    int min = atoi(argv[4]);
    int max = atoi(argv[5]);
    //open named pipe
    sensor_fd = open(SENSOR_FIFO, O_WRONLY);
    if (sensor_fd < 0)
    {
        printf("Error: Could not open sensor fifo\n");
        exit(1);
    }

    a_sigtstp.sa_handler = sigtstp_handler;
    sigfillset(&a_sigtstp.sa_mask);
    a_sigtstp.sa_flags = 0;
    sigaction(SIGTSTP, &a_sigtstp, NULL);

    a_sigint.sa_handler = sigint_handler;
    sigfillset(&a_sigint.sa_mask);
    a_sigint.sa_flags = 0;
    sigaction(SIGINT, &a_sigint, NULL);

    char buffer[128];
    while(1)
    {
        int value = random_number(min, max);
        //sleep for time seconds
        int x = sleep(time);
        while (x != 0)
        {
            x = sleep(x);
        }
        

        sprintf(buffer, "%s#%s#%d", id, key, value);
        printf("%s\n", buffer);
        //send to named pipe
        if(write(sensor_fd, &buffer, strlen(buffer)) < 0)
        {
            printf("Error: Could not write to sensor fifo\n");
            exit(1);
        }
        count++;
    }
    
    //SEND TO NAMED PIPE
    
    // char *fifo = "/tmp/pipe";
    // mkfifo(fifo, 0666);
    // FILE *fp;
    // fp = fopen(fifo, "w");
    // fprintf(fp, "%d#%s#%d", id, key, value);
    // fclose(fp);


    return 0;
}


