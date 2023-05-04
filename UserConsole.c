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
#include "UserCommand.h"

#define MAX_ARGS 4
#define MAX_ARG_LEN 32
#define CONSOLE_FIFO "/tmp/console_fifo"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Error: Expected 1 parameter, got %d\n", argc);
        exit(1);
    }

    //id parameter (SENS1) size must be between 3 and 32
    // if (strlen(argv[1]) < 3 || strlen(argv[1]) > 32)
    // {
    //     printf("Error: Expected first parameter to be between 3 and 32 characters, got %ld\n", strlen(argv[1]));
    //     exit(1);
    // }
    //only alfanumeric characters
    // for(size_t i = 0; i < strlen(argv[1]); i++)
    // {
    //     if (!isalnum(argv[1][i]))
    //     {
    //         printf("Error: Expected first parameter to be a string with characters and numbers, got %c\n", argv[1][i]);
    //         exit(1);
    //     }
    // }
    if (atoi(argv[1]) < 0 ){
        printf("Error: Expected parameter to be a positive number, got %d\n", atoi(argv[1]));
        exit(1);
    }
    int id = atoi(argv[1]);
    
    char input[100];
    char command[MAX_ARG_LEN+1];
    char args[MAX_ARGS][MAX_ARG_LEN+1];
    int num_args;
    
    int console_fd = open(CONSOLE_FIFO, O_WRONLY);
    if (console_fd < 0)
    {
        printf("Error: Could not open console fifo\n");
        exit(1);
    }

    while (1) {
        UserCommand user_command;
        user_command.console_id = id;
        printf("Enter command: ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';
        
        char *token = strtok(input, " ");
        if (token != NULL) {
            int e = 0;
            strcpy(command, token);
            num_args = 0;
            while ((token = strtok(NULL, " ")) != NULL && num_args < MAX_ARGS) {
                if(strlen(token) > MAX_ARG_LEN){
                    printf("Error: Expected parameter %d to be less than %d characters, got %ld\n", num_args, MAX_ARG_LEN, strlen(token));
                    e = 1;
                }
                strcpy(args[num_args], token);
                num_args++;
            }
            if(e){
                continue;
            }
        }

        if (strcmp(command, "exit") == 0)
        {
            if(num_args != 0){
                printf("Error: Expected 0 parameters, got %d\n", num_args);
                continue;
            }
            user_command.command = command;
            user_command.args = NULL;
            printf("sizeof(UserCommand) = %ld\n", USERCOMMANDSIZE);
            write(console_fd, &user_command, USERCOMMANDSIZE);
            break;

        }

        else if(strcmp(command,"stats") == 0){
            if(num_args != 0){
                printf("Error: Expected 0 parameters, got %d\n", num_args);
                continue;
            }
            printf("Key\tLast\tMin\tMax\tAvg\tCount\n");
            
            //UGLY: perguntar ao stor se este e o tamanho que tenho que enviar
        
            user_command.command = command;
            user_command.args = NULL;
            printf("sizeof(UserCommand) = %ld\n", USERCOMMANDSIZE);
            write(console_fd, &user_command, USERCOMMANDSIZE);
        }

        else if(strcmp(command,"sensors") == 0){
            if(num_args != 0){
                printf("Error: Expected 0 parameters, got %d\n", num_args);
                continue;
            }
            printf("ID\n");

            user_command.command = command;
            user_command.args = NULL;
            printf("sizeof(UserCommand) = %ld\n", USERCOMMANDSIZE);
            write(console_fd, &user_command, USERCOMMANDSIZE);
        }

        else if(strcmp(command,"reset") == 0){
            if(num_args != 0){
                printf("Error: Expected 0 parameters, got %d\n", num_args);
                continue;
            }
            printf("OK\n");
            
            user_command.command = command;
            user_command.args = NULL;
            printf("sizeof(UserCommand) = %ld\n", USERCOMMANDSIZE);
            write(console_fd, &user_command, USERCOMMANDSIZE);
        }

        else if (strcmp(command, "add_alert") == 0)
        {
            if(num_args != 4){
                printf("Error: Expected 4 parameters, got %d\n", num_args);
                continue;
            }
            char* alert_id = args[0];
            char* key = args[1];
            int min = atoi(args[2]);
            int max = atoi(args[3]);
            int err = 0;
            if (strlen(alert_id) < 3 || strlen(alert_id) > 32)
            {
                printf("Error: Expected first parameter to be between 3 and 32 characters, got %ld\n", strlen(alert_id));
                err = 1;
            }
            for (size_t i = 0; i < strlen(alert_id); i++)
            {
                if (!isalnum(alert_id[i]))
                {
                    printf("Error: Expected first parameter to be a string with characters and numbers, got %c\n", alert_id[i]);
                    err = 1;
                }
            }
            if (strlen(key) < 3 || strlen(key) > 32)
            {
                printf("Error: Expected second parameter to be between 3 and 32 characters, got %ld\n", strlen(key));
                err = 1;
            }
            for (size_t i = 0; i < strlen(key); i++){
                if (key[i] != '_' && !isalnum(key[i]))
                {
                    printf("Error: Expected second parameter to be a string with characters, numbers, and _, got %c\n", key[i]);
                    err = 1;
                }
            }
        
            if (min > max)
            {
                printf("Error: Expected third parameter has to be <= forth parameter, got %d and %d\n", min, max);
                err = 1;
            }
            if (err == 1)
            {
                continue;
            }
            printf("OK\n");

            user_command.command = command;
            user_command.num_args = num_args;
            user_command.args = (union arguments*) malloc(sizeof(union arguments)*(num_args));
            strcpy(user_command.args[0].argchar, alert_id);
            strcpy(user_command.args[1].argchar, key);
            user_command.args[2].argint = min;
            user_command.args[3].argint = max;
            printf("sizeof(UserCommand) = %ld\n", USERCOMMANDSIZE);
            write(console_fd, &user_command, USERCOMMANDSIZE);
            free(user_command.args);
        }

        else if (strcmp(command, "remove_alert") == 0)
        {
            if(num_args != 1){
                printf("Error: Expected 1 parameter, got %d\n", num_args);
                continue;
            }
            char *alert_id = args[0];
            int err = 0;
            if (strlen(alert_id) < 3 || strlen(alert_id) > 32)
            {
                printf("Error: Expected second parameter to be between 3 and 32 characters, got %ld\n", strlen(alert_id));
                err = 1;
            }
            for (size_t i = 0; i < strlen(alert_id); i++)
            {
                if (!isalnum(alert_id[i]))
                {
                    printf("Error: Expected second parameter to be a string with characters and numbers, got %c\n", alert_id[i]);
                    err = 1;
                }
            }
            if (err == 1)
            {
                continue;
            }
            printf("OK\n");

            user_command.command = command;
            user_command.args = alert_id;
            printf("sizeof(UserCommand) = %ld\n", USERCOMMANDSIZE);
            write(console_fd, &user_command, USERCOMMANDSIZE);
        }

        else if(strcmp(command,"list_alerts") == 0){
            if(num_args != 0){
                printf("Error: Expected 0 parameters, got %d\n", num_args);
                continue;
            }
            printf("ID\tKey\tMIN\tMAX\n");

            user_command.command = command;
            user_command.args = NULL;
            printf("sizeof(UserCommand) = %ld\n", USERCOMMANDSIZE);
            write(console_fd, &user_command, USERCOMMANDSIZE);
        }
        else{
            printf("Error: Unknown command %s\n", command);
        }
        printf("\n");
    }

    close(console_fd);

    return 0;
}