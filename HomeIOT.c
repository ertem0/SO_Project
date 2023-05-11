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
#include <time.h>
#include "Structs.h"
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/msg.h>
#include "InternalQueue.h"
#include "MessageQueue.h"

#define SENSOR_FIFO "/tmp/sensor_fifo"
#define CONSOLE_FIFO "/tmp/console_fifo"

int QUEUE_SZ;
int N_WORKERS;
int MAX_KEYS;
int MAX_SENSORS;
int MAX_ALERTS;
int shmid;
int fl;
int console_fd;
int sensor_fd;
int **upipefd;
int mqid;
SharedMEM *sharedmem;
pthread_t threads[3];
pid_t *wpids;
pid_t apid;
int num_forks;
InternalQueue *iq;
struct sigaction ignore;
struct sigaction ctrlc;
int is_working=0;
int is_running=1;

void lprint(const char* format, ...) {
    // check if file exists
    if (fl == -1)
    {
        printf("Error writting to file! File pointer is null\n");
        return;
    }

    time_t now = time(NULL);
    struct tm tm_struct;
    localtime_r(&now, &tm_struct);

    int hour = tm_struct.tm_hour;
    int min = tm_struct.tm_min;
    int sec = tm_struct.tm_sec;

    pthread_mutex_lock(&sharedmem->mutex_log);
    va_list args;
    va_start(args, format);
    char str[1024];
    char time[20];
    vsprintf(str, format, args);
    sprintf(time, "[%02d:%02d:%02d] ", hour, min, sec);
    write(fl, time, strlen(time));
    write(fl, str, strlen(str));
    printf("%s", time);
    printf("%s", str);
    va_end(args);
    pthread_mutex_unlock(&sharedmem->mutex_log);
}

void sysclose(){
    //close mutex
    //check if mutex is open
    if(&sharedmem->mutex != NULL){
        pthread_mutex_destroy(&sharedmem->mutex);
        printf("Mutex closed successfully\n");
    }
    if(&sharedmem->mutex_log != NULL){
        pthread_mutex_destroy(&sharedmem->mutex_log);
        printf("Log mutex closed successfully\n");
    }
    if(&sharedmem->mutex_worker_ready != NULL){
        pthread_mutex_destroy(&sharedmem->mutex_worker_ready);
        printf("Worker ready mutex closed successfully\n");
    }
    if(&sharedmem->mutex_wait_worker_ready != NULL){
        pthread_mutex_destroy(&sharedmem->mutex_wait_worker_ready);
        printf("Wait worker ready mutex closed successfully\n");
    }
    if(&sharedmem->mutex_iq != NULL){
        pthread_mutex_destroy(&sharedmem->mutex_iq);
        printf("Internal queue mutex closed successfully\n");
    }
    if(&sharedmem->mutex_keylist != NULL){
        pthread_mutex_destroy(&sharedmem->mutex_keylist);
        printf("Key list mutex closed successfully\n");
    }
    if(&sharedmem->mutex_sensorlist != NULL){
        pthread_mutex_destroy(&sharedmem->mutex_sensorlist);
        printf("Sensor list pipe mutex closed successfully\n");
    }
    if(&sharedmem->mutex_alertlist != NULL){
        pthread_mutex_destroy(&sharedmem->mutex_alertlist);
        printf("Alert list mutex closed successfully\n");
    }
    //close condition variable
    if(&sharedmem->cond != NULL){
        pthread_cond_destroy(&sharedmem->cond);
        printf("Condition variable closed successfully\n");
    }
    if(&sharedmem->cond_wait_worker_ready != NULL){
        pthread_cond_destroy(&sharedmem->cond_wait_worker_ready);
        printf("Worker ready condition variable closed successfully\n");
    }
    if(&sharedmem->cond_iq != NULL){
        pthread_cond_destroy(&sharedmem->cond_iq);
        printf("Internal queue condition variable closed successfully\n");
    }
    if(&sharedmem->cond_iq_freed != NULL){
        pthread_cond_destroy(&sharedmem->cond_iq_freed);
        printf("Internal queue freed condition variable closed successfully\n");
    }
    sharedmem->cond_alertlist.__data.__wrefs = 0;
    if(&sharedmem->cond_alertlist != NULL){
        pthread_cond_destroy(&sharedmem->cond_alertlist);
        printf("Alert List condition variable closed successfully\n");
    }
    //detach and delete shared memory
    if(sharedmem != NULL){
        shmdt(sharedmem);
        shmctl(shmid, IPC_RMID, NULL);
        printf("Shared memory deleted successfully\n");
    }
    //close log file
    //check if file is open
    if (fl != -1)
    {
        close(fl);
        printf("Log file closed successfully\n");
    }
    //close fifo
    if (sensor_fd != -1)
    {
        close(sensor_fd);
        printf("Sensor fifo closed successfully\n");
    }
    if (console_fd != -1)
    {
        close(console_fd);
        printf("Console fifo closed successfully\n");
    }
    unlink(SENSOR_FIFO);
    unlink(CONSOLE_FIFO);

    //close message queue
    if(mqid != -1){
        msgctl(mqid, IPC_RMID, NULL);
        printf("Message queue closed successfully\n");
    }

    if(iq != NULL)
        free_queue(iq);

    //TODO: free others
    if(wpids != NULL)
        free(wpids);

    if(upipefd != NULL)
        free(upipefd);

    for(int i = 1; i < N_WORKERS; i++){
        if(upipefd[i] != NULL)
            free(upipefd[i]);
    }
    

    exit(0);
}

void ctrlc_handler(){
    lprint("Received SIGINT. Closing program...\n");
    //close threads
    // is_running = 0;
    // pthread_cond_broadcast(&sharedmem->cond_wait_worker_ready);
    // pthread_cond_broadcast(&sharedmem->cond_iq);
    for(int i = 0; i < 3; i++){
        pthread_cancel(threads[i]);
        printf("Thread %d closed successfully\n", i);
    }

    //wait for processes to finish
    //TODO: ESPERAR PELOS PROCESSOS ACABAREM WORKER E se calhar alertwatcher
    /*here*/

    /*end*/

    //FIX: tenho de dar wait aos processes killed?
    for(int i = 0; i < num_forks; i++){
        wait(NULL);
    }
    wait(NULL);


    //print whats left in queue
    //FIX: dar close aos pipes antes ou depois de esperar pelos workers? para fechar logo os sensors e o user console
    
    sysclose();
}

int readConfigFile(char *filename)
{
    //read config file
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("Error opening file");
        return 0;
    }
    //read number from line
    int invalid = 0;


    fscanf(fp, "%d", &QUEUE_SZ);
    if(QUEUE_SZ < 1){
        printf("QUEUE_SZ must be greater than 0\n");
        invalid = 1;
    }
    printf("QUEUE_SZ = %d\n", QUEUE_SZ);

    fscanf(fp, "%d", &N_WORKERS);
    if(N_WORKERS < 1){
        printf("N_WORKERS must be greater than 0\n");
        invalid = 1;
    }
    printf("N_WORKERS = %d\n", N_WORKERS);

    fscanf(fp, "%d", &MAX_KEYS);
    if(MAX_KEYS < 1){
        printf("MAX_KEYS must be greater than 0\n");
        invalid = 1;
    }
    printf("MAX_KEYS = %d\n", MAX_KEYS);

    fscanf(fp, "%d", &MAX_SENSORS);
    if(MAX_SENSORS < 1){
        printf("MAX_SENSORS must be greater than 0\n");
        invalid = 1;
    }
    printf("MAX_SENSORS = %d\n", MAX_SENSORS);

    fscanf(fp, "%d", &MAX_ALERTS);
    if(MAX_ALERTS < 0){
        printf("MAX_ALERTS must be greater than or equal to 0\n");
        invalid = 1;
    }
    printf("MAX_ALERTS = %d\n", MAX_ALERTS);
    
    fclose(fp);
    
    if(invalid){
        return 0;
    }
    return 1;
}

void alertWatcher_sigint(){

    if(is_working){
        lprint("Alert Watcher is working. Waiting for it to finish...\n");
        is_running = 0;
    }
    else{
        lprint("Alert Watcher finished\n");
        
        exit(0);
    }
}

void alertWatcher(){
    struct sigaction act;
    act.sa_handler = alertWatcher_sigint;
    sigaction(SIGINT, &act, NULL);

    lprint("Alert Watcher started\n");

    pthread_mutex_lock(&sharedmem->mutex_alertlist);
    while(is_running){

        is_working = 0;
        pthread_cond_wait(&sharedmem->cond_alertlist, &sharedmem->mutex_alertlist);
        is_working = 1;
        //todo: search de todas
        int size;
        AlertNode **alerts = get_alerts(&sharedmem->alertlist, sharedmem->alertlist.modified, &size);
        if(size == 0){
            lprint("Error searching for alert (%s)\n", sharedmem->alertlist.modified);
            continue;
        }
        pthread_mutex_lock(&sharedmem->mutex_keylist);
        KeyNode *key = search(&sharedmem->keylist, sharedmem->alertlist.modified);
        if(key == NULL){
            lprint("Error searching for key (%s)\n", sharedmem->alertlist.modified);
            pthread_mutex_unlock(&sharedmem->mutex_keylist);
            continue;
        }
        KeyNode *tempKey = malloc(sizeof(KeyNode));
        memcpy(tempKey, key, sizeof(KeyNode));
        printf("!!!Alert Watcher found key %s\n", key->key);
        printf("<><><><><><> size: %d\n", size);
        pthread_mutex_unlock(&sharedmem->mutex_keylist);
        for(int i=0; i<size; i++){
            printf("!!!Alert Watcher found alert %s\n", alerts[i]->id);
            printf("%d>%d || %d<%d\n", alerts[i]->min, tempKey->last, alerts[i]->max, tempKey->last);
            if(alerts[i]->min > tempKey->last || alerts[i]->max < tempKey->last){
                MQMessage msg;
                msg.mtype = alerts[i]->console_id;
                sprintf(msg.mtext, "ALERT %s (%s %d TO %d) TRIGGERED\n", alerts[i]->id, alerts[i]->key, alerts[i]->min, alerts[i]->max);
                lprint("%s", msg.mtext);
                if (msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0) == -1) {
                    perror("msgsnd");
                }
            }
        }
        free(alerts);
        free(tempKey);
    }
    pthread_mutex_unlock(&sharedmem->mutex_alertlist);
    //read from shared memory
    //check if key is valid
    //if key is valid, send to sensor
    //if key is invalid, send to alert

    printf("Alert Watcher finished\n");
    return;
}

void worker_sigint(){
    if(is_working){
        lprint("Worker %d is working. Waiting for it to finish...\n", getpid());
        is_running = 0;
    }
    else{
        lprint("Worker %d finished\n", getpid());
        exit(0);
    }
}

void worker(int id){
    //set sigaction to default sigint
    struct sigaction act;
    act.sa_handler = worker_sigint;
    sigaction(SIGINT, &act, NULL);

    lprint("Worker %d started\n", getpid());

    close(upipefd[id][1]);

    Payload payload;

    while(is_running){
        pthread_mutex_lock(&sharedmem->mutex_wait_worker_ready);
        pthread_mutex_lock(&sharedmem->mutex_worker_ready);
        sharedmem->worker_ready[id] = 1;
        pthread_mutex_unlock(&sharedmem->mutex_worker_ready);
        pthread_cond_signal(&sharedmem->cond_wait_worker_ready);
        pthread_mutex_unlock(&sharedmem->mutex_wait_worker_ready);

        read(upipefd[id][0], &payload, sizeof(Payload));

        is_working = 1;
        
        //wait for signal
        if(payload.type == TYPE_USER_COMMAND){
            printf("Worker %d received %s\n", getpid(), payload.data.user_command.command);
            if(strcmp(payload.data.user_command.command, "STATS") == 0){
                pthread_mutex_lock(&sharedmem->mutex_keylist);
                char *allstats = malloc(sizeof(char)*128*sharedmem->keylist.size);
                allstats[0] = '\0';
                for(int i=0; i<sharedmem->keylist.size; i++){
                    //create a sting with the stats
                    char *stats = malloc(sizeof(char)*128);
                    sprintf(stats, "%s %d %d %d %f %d\n", sharedmem->keylist.nodes[i].key, sharedmem->keylist.nodes[i].last, sharedmem->keylist.nodes[i].min, sharedmem->keylist.nodes[i].max, sharedmem->keylist.nodes[i].mean, sharedmem->keylist.nodes[i].count);
                    strcat(allstats, stats);
                    free(stats);
                }
                pthread_mutex_unlock(&sharedmem->mutex_keylist);
                printf("%s\n", allstats);
                MQMessage msg;
                msg.mtype = payload.data.user_command.console_id;
                strcpy(msg.mtext, allstats);
                if (msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0) == -1) {
                    perror("msgsnd");
                }
                free(allstats);
            } 
            else if(strcmp(payload.data.user_command.command, "SENSORS") == 0){
                pthread_mutex_lock(&sharedmem->mutex_sensorlist);
                char *allsensors = malloc(sizeof(char)*128*sharedmem->sensorlist.size);
                allsensors[0] = '\0';
                printf("allsensors (%s)", allsensors);
                printf("sensorlist size (%d)", sharedmem->sensorlist.size);
                for(int i=0; i<sharedmem->sensorlist.size; i++){
                    //create a sting with the stats
                    char *sensor = malloc(sizeof(char)*128);
                    sprintf(sensor, "%s\n", sharedmem->sensorlist.nodes[i].id);
                    strcat(allsensors, sensor);
                    printf("sensor (%s)", sensor);
                    free(sensor);
                }
                pthread_mutex_unlock(&sharedmem->mutex_sensorlist);
                printf("%s\n", allsensors);
                MQMessage msg;
                msg.mtype = payload.data.user_command.console_id;
                strcpy(msg.mtext, allsensors);
                if (msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0) == -1) {
                    perror("msgsnd");
                }
                free(allsensors);
    
            }
            else if(strcmp(payload.data.user_command.command, "LIST_ALERTS") == 0){
                pthread_mutex_lock(&sharedmem->mutex_alertlist);
                char *allalerts = malloc(sizeof(char)*128*sharedmem->alertlist.size);
                allalerts[0] = '\0';
                for(int i=0; i<sharedmem->alertlist.size; i++){
                    //create a sting with the stats
                    char *alert = malloc(sizeof(char)*128);
                    sprintf(alert, "%s %s %d %d\n", sharedmem->alertlist.nodes[i].id, sharedmem->alertlist.nodes[i].key, sharedmem->alertlist.nodes[i].min, sharedmem->alertlist.nodes[i].max);
                    strcat(allalerts, alert);
                    free(alert);
                }
                pthread_mutex_unlock(&sharedmem->mutex_alertlist);
                MQMessage msg;
                msg.mtype = payload.data.user_command.console_id;
                strcpy(msg.mtext, allalerts);
                if (msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0) == -1) {
                    perror("msgsnd");
                }
                free(allalerts);
            }
            else if(strcmp(payload.data.user_command.command, "RESET") == 0){
                pthread_mutex_lock(&sharedmem->mutex_keylist);
                sharedmem->keylist.size = 0;
                pthread_mutex_unlock(&sharedmem->mutex_keylist);
                MQMessage msg;
                msg.mtype = payload.data.user_command.console_id;
                strcpy(msg.mtext, "OK\n");
                if (msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0) == -1) {
                    perror("msgsnd");
                }

            }
            else if(strcmp(payload.data.user_command.command, "REMOVE_ALERT") == 0){
                MQMessage msg;
                pthread_mutex_lock(&sharedmem->mutex_alertlist);
                int status = remove_alert(&sharedmem->alertlist, payload.data.user_command.args[0].argchar);
                pthread_mutex_unlock(&sharedmem->mutex_alertlist);
                if(status == 0){
                    lprint("Error removing alert (%s)\n", payload.data.user_command.args[0].argchar);
                    msg.mtype = payload.data.user_command.console_id;
                    strcpy(msg.mtext, "ERROR\n");
                    if (msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0) == -1) {
                        perror("msgsnd");
                    }
                }
                else if(status == 1){
                    lprint("Alert %s removed successfully\n", payload.data.user_command.args[0].argchar);
                    msg.mtype = payload.data.user_command.console_id;
                    strcpy(msg.mtext, "OK\n");
                    if (msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0) == -1) {
                        perror("msgsnd");
                    }
                }
            }
            else if(strcmp(payload.data.user_command.command, "ADD_ALERT") == 0){
                MQMessage msg;
                pthread_mutex_lock(&sharedmem->mutex_alertlist);
                int status = insert_alert(&sharedmem->alertlist, payload.data.user_command.args[0].argchar, payload.data.user_command.args[1].argchar, payload.data.user_command.args[2].argint, payload.data.user_command.args[3].argint, payload.data.user_command.console_id);
                pthread_mutex_unlock(&sharedmem->mutex_alertlist);
                if(status == 0){
                    lprint("Error inserting alert (%s)\n", payload.data.user_command.args[0].argchar, sharedmem->alertlist.max_size, sharedmem->alertlist.size);
                    msg.mtype = payload.data.user_command.console_id;
                    strcpy(msg.mtext, "ERROR\n");
                    if (msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0) == -1) {
                        perror("msgsnd");
                    }
                }
                else if(status == 1){
                    lprint("Alert %s added successfully\n", payload.data.user_command.args[0].argchar);
                    msg.mtype = payload.data.user_command.console_id;
                    strcpy(msg.mtext, "OK\n");
                    if (msgsnd(mqid, &msg, sizeof(msg)-sizeof(long), 0) == -1) {
                        perror("msgsnd");
                    }
                }
            }
        }

        else if(payload.type == TYPE_SENSOR_DATA){
            printf("Worker %d received %s\n", getpid(), payload.data.sensor_data.id);
            int status_key, status_sensor;
            
            pthread_mutex_lock(&sharedmem->mutex_sensorlist);
            status_sensor = insert_sensor(&sharedmem->sensorlist, payload.data.sensor_data.id);
            if(status_sensor == 0){
                lprint("Error inserting sensor (%s) max size reached (max: %d, current:%d)\n", payload.data.sensor_data.id, sharedmem->sensorlist.max_size, sharedmem->sensorlist.size);
            }
            else if(status_sensor == 1){
                lprint("Sensor already in list (%s)\n", payload.data.sensor_data.id);
            }
            else if(status_sensor == 2){
                lprint("Sensor %s added successfully\n", sharedmem->sensorlist.nodes[sharedmem->sensorlist.size-1].id);
            }
            pthread_mutex_unlock(&sharedmem->mutex_sensorlist);
            printf("Worker %d inserting key %s\n", getpid(), payload.data.sensor_data.key);
            pthread_mutex_lock(&sharedmem->mutex_keylist);
                status_key = insert(&sharedmem->keylist, payload.data.sensor_data.key, payload.data.sensor_data.value);
            pthread_mutex_unlock(&sharedmem->mutex_keylist);
            pthread_mutex_lock(&sharedmem->mutex_alertlist);
                strcpy(sharedmem->alertlist.modified, payload.data.sensor_data.key);
                pthread_cond_signal(&sharedmem->cond_alertlist);
            pthread_mutex_unlock(&sharedmem->mutex_alertlist);
            printf("Worker %d inserted key %s code: %d\n", getpid(), payload.data.sensor_data.key, status_key);
            if(status_key == 0){
                lprint("Error inserting key (%s) max size reached (max: %d, current:%d)\n", payload.data.sensor_data.key, sharedmem->keylist.max_size, sharedmem->keylist.size);
            }
            else if(status_key == 1){
                lprint("Key %s updated successfully\n", payload.data.sensor_data.key);
            }
            else if(status_key == 2){
                lprint("Key %s added successfully\n", payload.data.sensor_data.key);
            }

            
        }
        is_working = 0;

        printf("Worker %d finished task\n", getpid());
    }
    

    //read from shared memory
    //check if key is valid
    //if key is valid, send to sensor
    //if key is invalid, send to alert

    lprint("Worker %d finished\n", getpid());
    return;
}

void *dispacher(void *arg){
    long my_id = (long)arg;
    lprint("Dispacher %ld started\n", my_id);

    //create unamed pipe
    

    Payload *payload;
    //wait for signal
    //TODO: change while condition to a signal to end program
    //FIXME: when no workers ready cant add to queue
    pthread_mutex_lock(&sharedmem->mutex_wait_worker_ready);
    while (1) //is_running)
    {
        int found = -1;
        //get which worker is ready
        for(int i = 0; i < N_WORKERS; i++){
            pthread_mutex_lock(&sharedmem->mutex_worker_ready);
            if(sharedmem->worker_ready[i] == 1){
                printf("Dispacher %ld found worker %d ready\n", my_id, wpids[i]);
                pthread_mutex_unlock(&sharedmem->mutex_worker_ready);
                found = i;
                break;
            }
            pthread_mutex_unlock(&sharedmem->mutex_worker_ready);
        }
        while(found == -1){
            printf("Dispacher %ld waiting for worker\n", my_id);
            pthread_cond_wait(&sharedmem->cond_wait_worker_ready, &sharedmem->mutex_wait_worker_ready);

            for(int i = 0; i < N_WORKERS; i++){
                pthread_mutex_lock(&sharedmem->mutex_worker_ready);
                if(sharedmem->worker_ready[i] == 1){
                    printf("Dispacher %ld found worker %d ready\n", my_id, wpids[i]);
                    pthread_mutex_unlock(&sharedmem->mutex_worker_ready);
                    found = i;
                    break;
                }
                pthread_mutex_unlock(&sharedmem->mutex_worker_ready);
            }

            // if(is_running == 0){
            //     pthread_mutex_unlock(&sharedmem->mutex_wait_worker_ready);
            //     printf("Dispacher %ld finished\n", my_id);
            //     pthread_exit(NULL);
            // }
        }
        
        pthread_mutex_lock(&sharedmem->mutex_iq);
        printf("Dispacher %ld waiting for iq\n", my_id);
        while(is_empty(iq)){
            pthread_cond_wait(&sharedmem->cond_iq, &sharedmem->mutex_iq);
            
            // if(is_running == 0){
            //     pthread_mutex_unlock(&sharedmem->mutex_iq);
            //     pthread_mutex_unlock(&sharedmem->mutex_wait_worker_ready);
            //     printf("Dispacher %ld finished\n", my_id);
            //     pthread_exit(NULL);
            // }
        }
        payload = remove_message(iq);
        pthread_cond_signal(&sharedmem->cond_iq_freed);
        if(payload == NULL){
            printf("Dispacher %ld received NULL\n", my_id);
            pthread_mutex_unlock(&sharedmem->mutex_iq);
            continue;
        }
        pthread_mutex_unlock(&sharedmem->mutex_iq);
        
        printf("Dispacher %ld received %d\n", my_id, payload->type);
        
        printf("Dispacher %ld sending to worker %d\n", my_id, wpids[found]);
        //UGLY: perguntar ao stor sobre o tamanho do buffer e se isto e necessario
        Payload to_send;
        memcpy(&to_send, payload, sizeof(Payload));
        printf("copyied payload %s\n", to_send.type ? to_send.data.user_command.command : to_send.data.sensor_data.id);

        write(upipefd[found][1], &to_send, sizeof(to_send));
        pthread_mutex_lock(&sharedmem->mutex_worker_ready);
        sharedmem->worker_ready[found] = 0;
        pthread_mutex_unlock(&sharedmem->mutex_worker_ready);


        free(payload);
    }
    pthread_mutex_unlock(&sharedmem->mutex_wait_worker_ready);
    
    
    //lock mutex
    //check queue
    //if queue is empty, unlock mutex and wait for signal
    //if queue is not empty, remove from queue and unlock mutex
    //check if key is valid
    //if key is valid, send to worker
    //if key is invalid, send to alert

    lprint("Dispacher %ld finished\n", my_id);
    pthread_exit(NULL);
}

void *consoleReader(void *arg){
    long my_id = (long)arg;
    lprint("Console Reader %ld started\n", my_id);

    // Opens the pipe for reading
    if ((console_fd = open(CONSOLE_FIFO, O_RDONLY)) < 0) {
        perror("Cannot open pipe for reading: ");
        sysclose();
    }
    printf("console_fifo opened\n");

    ssize_t bytes_read;
    UserCommand received;
    //read from console
    while(1){
        bytes_read = read(console_fd, &received, sizeof(received));
        if(bytes_read < 0){
            perror("Error reading from console: ");
        }
        else if(bytes_read == 0){
            //printf("Console closed\n");
            //break;
        }
        else {
            // buffer[bytes_read] = '\0';
            Payload *payload = malloc(sizeof(Payload));
            payload->type = TYPE_USER_COMMAND;
            payload->data.user_command = received;
            printf("Read from console: %s\n", payload->data.user_command.command);
            //add to queue
            pthread_mutex_lock(&sharedmem->mutex_iq);
            while(add_message(iq, payload, 1) == -1){
                lprint("Queue is full, message (%s) not added\n", received.command);
                //TODO: wait until queue is not full
                pthread_cond_wait(&sharedmem->cond_iq_freed, &sharedmem->mutex_iq);
            }
            pthread_cond_signal(&sharedmem->cond_iq);
            pthread_mutex_unlock(&sharedmem->mutex_iq);
            //signal dispacher
            //wait for worker
            //print iq
            list_messages(iq);
        }
    }

    //close pipe
    close(console_fd);
    unlink(CONSOLE_FIFO);

    lprint("Console Reader %ld finished\n", my_id);
    pthread_exit(NULL);
}

void *sensorReader(void *arg){
    long my_id = (long)arg;
    lprint("Sensor Reader %ld started\n", my_id);

    if ((sensor_fd = open(SENSOR_FIFO, O_RDONLY)) < 0) {
        perror("Cannot open pipe for reading: ");
        sysclose();
    }
    printf("sensor_fifo opened\n");

    ssize_t bytes_read;
    char buffer[128];

    while(1){
        bytes_read = read(sensor_fd, &buffer, sizeof(buffer));
        if(bytes_read < 0){
            perror("Error reading from console: ");
        }
        else if(bytes_read == 0){
            //printf("Console closed\n");
            //break;
        }
        else {
            buffer[bytes_read] = '\0';
            printf("Read from sensor: %s\n", buffer);
            Payload *payload = malloc(sizeof(Payload));
            payload->type = TYPE_SENSOR_DATA;
            //split a string of this format x#x#x
            char *token = strtok(buffer, "#");
            int i = 0;
            while(token != NULL){
                if(i == 0){
                    strcpy(payload->data.sensor_data.id, token);
                }
                else if(i == 1){
                    strcpy(payload->data.sensor_data.key, token);
                }
                else if(i == 2){
                    payload->data.sensor_data.value = atoi(token);
                }
                else{
                    lprint("Error reading from sensor\n");
                    break;
                }
                token = strtok(NULL, "#");
                i++;
            }


            printf("Read from sensor: %s\n", payload->data.sensor_data.id);
            //add to queue
            pthread_mutex_lock(&sharedmem->mutex_iq);
            if(add_message(iq, payload, 2) == -1){
                lprint("Queue is full, message (%s) not added\n", buffer);
                //TODO: wait until queue is not full
            }
            pthread_cond_signal(&sharedmem->cond_iq);
            pthread_mutex_unlock(&sharedmem->mutex_iq);
            //signal dispacher
            //wait for worker
            //print iq
            list_messages(iq);
        }
    }


    //read from sensor
    //add to queue
    //signal dispacher

    lprint("Sensor Reader %ld finished\n", my_id);
    pthread_exit(NULL);
}

void systemManager(){
    long id[3]={0,1,2};
    
    iq = create_queue(QUEUE_SZ);

    upipefd = (int **)malloc(sizeof(int*)*N_WORKERS);
    if(upipefd == NULL){
        perror("Error allocating memory for unamed pipe");
        //close system
        return;
    }
    for(int i = 0; i < N_WORKERS; i++){
        upipefd[i] = (int*)malloc(sizeof(int)*2);
    }

    for(int i = 0; i < N_WORKERS; i++){
        if(pipe(upipefd[i]) == -1){
            perror("Pipe failed");
            //close system
            return;
        }
    }

    //create threads consoleReader, sensorReader, dispacher
    if(pthread_create(&threads[0], NULL, consoleReader, (void *)id[0]) != 0){
        perror("Error creating console reader thread");
        //fechar o sys
        return;
    }
    if(pthread_create(&threads[1], NULL, sensorReader, (void *)id[1]) != 0){
        perror("Error creating sensor reader thread");
        //fechar o sys
        //close threads[0]
        pthread_cancel(threads[0]);
        return;
    }
    if(pthread_create(&threads[2], NULL, dispacher, (void *)id[2]) != 0){
        perror("Error creating dispacher thread");
        //fechar o sys
        //close threads[0] and threads[1]
        pthread_cancel(threads[0]);
        pthread_cancel(threads[1]);
        return;
    }

    wpids = malloc(sizeof(pid_t) * N_WORKERS);
    num_forks = 0;

    //sigaction(SIGINT, &ignore, NULL);
    for (int i = 0; i < N_WORKERS; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            worker(i);
            exit(0);
        } else if (pid > 0) {
            wpids[num_forks++] = pid;
        } else {
            // Failed to fork
            perror("Failed to init worker\n");

            for (int j = 0; j < num_forks; j++) {
                kill(wpids[j], SIGTERM);
            }
            for(int j = 0; j < 3; j++){
                pthread_cancel(threads[j]);
            }
            //sysclose
            return;
        }
    }

    apid = fork();
    if(apid==0){
        alertWatcher();
        exit(0);
    }else if(apid==-1){
        perror("Error creating alert watcher");
        for(int i = 0; i < N_WORKERS; i++){
            kill(wpids[i], SIGTERM);
        }
        for(int i = 0; i < 3; i++){
            pthread_cancel(threads[i]);
        }
        //fechar o sys
        return;
    }

    //sigaction(SIGINT, &ctrlc, NULL);

    //wait for threads to finish
    for(int i = 0; i < 3; i++){
        pthread_join(threads[i], NULL);
    }

    //wait for workers to finish
    for(int i = 0; i < N_WORKERS; i++){
        wait(NULL);
    }
    //wait for alert watcher to finish
    wait(NULL);

    lprint("System Manager finished\n");
}

int main(int argc, char *argv[])
{
    //check for correct number of arguments
    if(argc != 2){
        fprintf(stderr, "Usage: ./HomeIOT <config file>: Too many or too few arguments\n");
        return 0;
    }
    
    ctrlc.sa_handler = ctrlc_handler;
    sigfillset(&ctrlc.sa_mask);
    ctrlc.sa_flags = 0;
    sigaction(SIGINT, &ctrlc, NULL);
    
    ignore.sa_handler = SIG_IGN;
    sigfillset(&ignore.sa_mask);
    ignore.sa_flags = 0;

    //opening log file
    fl = open("log.txt", O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if(fl == -1){
        perror("Error opening log file");
        return 0;
    }

    //read config file
    if(readConfigFile(argv[1])){
        printf("Config file read successfully\n");
    }
    else{
        perror("Config file read failed");
        sysclose();
        return 0;
    }

    //create shared memory of SharedMEM struct
    shmid = shmget(IPC_PRIVATE, sizeof(SharedMEM) + sizeof(int)*N_WORKERS + sizeof(KeyNode) * MAX_KEYS + sizeof(SensorNode)*MAX_SENSORS + sizeof(AlertNode)*MAX_ALERTS, IPC_CREAT | 0666);
    if (shmid < 0)
    {
        perror("shmget");
        //fechar o q tenho ate agora (acho q ta tudo)
        close(fl);
        return 0;
    }
    sharedmem = (SharedMEM *)shmat(shmid, NULL, 0);
    if (sharedmem == (SharedMEM *)-1)
    {
        perror("shmat");
        //fechar o q tenho ate  (acho q ta tudo)
        shmctl(shmid, IPC_RMID, NULL);
        close(fl);
        return 0;
    }
    lprint("Shared memory created\n");
    sharedmem->worker_ready = (int*)(((void*)sharedmem) + sizeof(SharedMEM));
    for(int i = 0; i < N_WORKERS; i++){
        sharedmem->worker_ready[i] = 0;
    }

    sharedmem->keylist.nodes = (KeyNode*)(((void*)sharedmem->worker_ready) + sizeof(int)*N_WORKERS);
    printf("keylist nodes: %p\n", sharedmem->keylist.nodes);
    sharedmem->keylist.size = 0;
    sharedmem->keylist.max_size = MAX_KEYS;
    //print start pointer of each node
    printf("node size %d\n", sizeof(KeyNode));
    for(int i = 0; i < MAX_KEYS; i++){
        printf("keylist node %d: %ld\n", i, &sharedmem->keylist.nodes[i]);
    }
    
    sharedmem->sensorlist.nodes = (SensorNode*)(((void*)sharedmem->keylist.nodes) + (sizeof(KeyNode)*MAX_KEYS));
    printf("sensorlist nodes: %p\n", sharedmem->sensorlist.nodes);
    sharedmem->sensorlist.size = 0;
    sharedmem->sensorlist.max_size = MAX_SENSORS;
    printf("node size %d\n", sizeof(SensorNode));
    for(int i = 0; i < MAX_SENSORS; i++){
        printf("sensorlist node %d: %ld\n", i, &sharedmem->sensorlist.nodes[i]);
    }
    
    sharedmem->alertlist.nodes = (AlertNode*)(((void*)sharedmem->sensorlist.nodes) + sizeof(SensorNode) * MAX_SENSORS);
    printf("alertlist nodes: %p\n", sharedmem->alertlist.nodes);
    sharedmem->alertlist.size = 0;
    sharedmem->alertlist.max_size = MAX_ALERTS;
    printf("node size %d\n", sizeof(AlertNode));
    for(int i = 0; i < MAX_ALERTS; i++){
        printf("alertlist node %d: %ld\n", i, &sharedmem->alertlist.nodes[i]);
    }
    
    //create mutex
    pthread_mutexattr_t att;
    pthread_mutexattr_init(&att);
    //pthread_mutexattr_setrobust(&att, PTHREAD_MUTEX_STALLED);
    pthread_mutexattr_setpshared(&att, PTHREAD_PROCESS_SHARED);
    
    if (pthread_mutex_init(&sharedmem->mutex, &att) != 0) {
        printf("Failed to initialize mutex.\n");
        sysclose();
        return 1;
    }
    if(pthread_mutex_init(&sharedmem->mutex_log, &att) != 0){
        printf("Failed to initialize log mutex.\n");
        sysclose();
        return 1;
    }
    if(pthread_mutex_init(&sharedmem->mutex_worker_ready, &att) != 0){
        printf("Failed to initialize worker ready mutex.\n");
        sysclose();
        return 1;
    }
    if(pthread_mutex_init(&sharedmem->mutex_wait_worker_ready, &att) != 0){
        printf("Failed to initialize worker pipe mutex.\n");
        sysclose();
        return 1;
    }
    if(pthread_mutex_init(&sharedmem->mutex_iq, &att) != 0){
        printf("Failed to initialize iq mutex.\n");
        sysclose();
        return 1;
    }
    if(pthread_mutex_init(&sharedmem->mutex_keylist, &att) != 0){
        printf("Failed to initialize alert mutex.\n");
        sysclose();
        return 1;
    }
    if(pthread_mutex_init(&sharedmem->mutex_sensorlist, &att) != 0){
        printf("Failed to initialize alert mutex.\n");
        sysclose();
        return 1;
    }
    if(pthread_mutex_init(&sharedmem->mutex_alertlist, &att) != 0){
        printf("Failed to initialize alert mutex.\n");
        sysclose();
        return 1;
    }

    lprint("Mutex created\n");
    //create semaphores
    //maybe not needed

    //create condition variables
    pthread_condattr_t condatt;
    pthread_condattr_init(&condatt);
    pthread_condattr_setpshared(&condatt, PTHREAD_PROCESS_SHARED);
    if(pthread_cond_init(&sharedmem->cond, &condatt) != 0){
        printf("Failed to initialize condition variable.\n");
        sysclose();
        return 1;
    }
    if(pthread_cond_init(&sharedmem->cond_wait_worker_ready, &condatt) != 0){
        printf("Failed to initialize condition variable.\n");
        sysclose();
        return 1;
    }
    if(pthread_cond_init(&sharedmem->cond_iq, &condatt) != 0){
        printf("Failed to initialize condition variable.\n");
        sysclose();
        return 1;
    }
    if(pthread_cond_init(&sharedmem->cond_iq_freed, &condatt) != 0){
        printf("Failed to initialize condition variable.\n");
        sysclose();
        return 1;
    }
    if(pthread_cond_init(&sharedmem->cond_alertlist, &condatt) != 0){
        printf("Failed to initialize condition variable.\n");
        sysclose();
        return 1;
    }
    lprint("Condition variable created\n");

    // Creates the named pipe if it doesn't exist yet
    if ((mkfifo(CONSOLE_FIFO, O_CREAT|O_EXCL|0600)<0)) {
        perror("Cannot create pipe: ");
        sysclose();
    }
    
    if(mkfifo(SENSOR_FIFO, O_CREAT|O_EXCL|0600)<0){
        perror("Cannot create pipe: ");
        sysclose();
    }
    
    lprint("FIFOs created\n");

    //create message queue
    mqid = msgget(MSG_KEY, IPC_CREAT|0700);
    if(mqid == -1){
        perror("Error creating message queue");
        sysclose();
    }
    

    systemManager();

    sysclose();
    return 0;
}


//diferneciar o q veio do console e o que veio do sensor