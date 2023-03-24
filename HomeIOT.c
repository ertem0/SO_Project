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
#include "SharedMEM.h"

int QUEUE_SZ;
int N_WORKERS;
int MAX_KEYS;
int MAX_SENSORS;
int MAX_ALERTS;
FILE *fl;
int shmid;
SharedMEM *sharedmem;

void lprint(const char* format, ...) {
    // check if file exists
    if (fl == NULL)
    {
        printf("Error writting to file! File pointer is null\n");
        exit(1);
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
    printf("[%d:%d:%d] ", hour, min, sec);
    vprintf(format, args);
    fprintf(fl, "[%d:%d:%d] ", hour, min, sec);
    vfprintf(fl, format, args);
    va_end(args);
    pthread_mutex_unlock(&sharedmem->mutex_log);
}

void sysclose(){
    //close log file
    fclose(fl);
    lprint("Log file closed successfully\n");
    //close mutex
    pthread_mutex_destroy(&sharedmem->mutex);
    pthread_mutex_destroy(&sharedmem->mutex_log);
    lprint("Mutexs closed successfully\n");
    //close condition variable
    pthread_cond_destroy(&sharedmem->cond);
    lprint("Condition variable closed successfully\n");
    //detach and delete shared memory
    shmdt(sharedmem);
    shmctl(shmid, IPC_RMID, NULL);
    printf("Shared memory deleted successfully\n");

    return;
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

    //TODO: verificar fscanf ler inteiro
    fscanf(fp, "%d", &QUEUE_SZ);
    if(QUEUE_SZ < 1){
        lprint("QUEUE_SZ must be greater than 0\n");
        invalid = 1;
    }
    lprint("QUEUE_SZ = %d\n", QUEUE_SZ);

    fscanf(fp, "%d", &N_WORKERS);
    if(N_WORKERS < 1){
        lprint("N_WORKERS must be greater than 0\n");
        invalid = 1;
    }
    lprint("N_WORKERS = %d\n", N_WORKERS);

    fscanf(fp, "%d", &MAX_KEYS);
    if(MAX_KEYS < 1){
        lprint("MAX_KEYS must be greater than 0\n");
        invalid = 1;
    }
    lprint("MAX_KEYS = %d\n", MAX_KEYS);

    fscanf(fp, "%d", &MAX_SENSORS);
    if(MAX_SENSORS < 1){
        lprint("MAX_SENSORS must be greater than 0\n");
        invalid = 1;
    }
    lprint("MAX_SENSORS = %d\n", MAX_SENSORS);

    fscanf(fp, "%d", &MAX_ALERTS);
    if(MAX_ALERTS < 0){
        lprint("MAX_ALERTS must be greater than or equal to 0\n");
        invalid = 1;
    }
    lprint("MAX_ALERTS = %d\n", MAX_ALERTS);
    
    fclose(fp);
    
    if(invalid){
        return 0;
    }
    return 1;
}

void alertWatcher(){
    lprint("Alert Watcher started\n");

    //read from shared memory
    //check if key is valid
    //if key is valid, send to sensor
    //if key is invalid, send to alert

    lprint("Alert Watcher finished\n");
    return;
}

void worker(){
    lprint("Worker %d started\n", getpid());

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

    //wait for signal
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

    //read from console
    //add to queue
    //signal dispacher

    lprint("Console Reader %ld finished\n", my_id);
    pthread_exit(NULL);
}

void *sensorReader(void *arg){
    long my_id = (long)arg;
    lprint("Sensor Reader %ld started\n", my_id);

    //read from sensor
    //add to queue
    //signal dispacher

    lprint("Sensor Reader %ld finished\n", my_id);
    pthread_exit(NULL);
}

void systemManager(){
    long id[3]={0,1,2};
    pthread_t threads[3];
    //create threads consoleReader, sensorReader, dispacher
    pthread_create(&threads[0], NULL, consoleReader, (void *)id[0]);
    pthread_create(&threads[1], NULL, sensorReader, (void *)id[1]);
    pthread_create(&threads[2], NULL, dispacher, (void *)id[2]);

    for(int i = 0; i < N_WORKERS; i++){ 
        pid_t pid = fork();
        if(pid==0){
            worker();
            exit(0);
        }else if(pid==-1){
            perror("Error creating worker");
            exit(0);
        }
    }

    pid_t pid = fork();
    if(pid==0){
        alertWatcher();
        exit(0);
    }else if(pid==-1){
        perror("Error creating alert watcher");
        exit(0);
    }

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
    
    //opening log file
    fl = fopen("log.txt", "w");
    if (fl == NULL)
    {
        perror("Error opening log file");
        return 0;
    }
    
    //create shared memory of SharedMEM struct
    shmid = shmget(IPC_PRIVATE, sizeof(SharedMEM), IPC_CREAT | 0666);
    if (shmid < 0)
    {
        perror("shmget");
        //TODO: fechar o q tenho ate agora (acho q ta tudo)
        fclose(fl);
        return 0;
    }    
    sharedmem = (SharedMEM *)shmat(shmid, NULL, 0);
    if (sharedmem == (SharedMEM *)-1)
    {
        perror("shmat");
        //TODO: fechar o q tenho ate  (acho q ta tudo)
        fclose(fl);
        shmctl(shmid, IPC_RMID, NULL);
        return 0;
    }
    lprint("Shared memory created successfully\n");

    //create mutex
    pthread_mutexattr_t att;
    pthread_mutexattr_init(&att);
    //pthread_mutexattr_setrobust(&att, PTHREAD_MUTEX_STALLED);
    pthread_mutexattr_setpshared(&att, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&sharedmem->mutex, &att);
    pthread_mutex_init(&sharedmem->mutex_log, &att);

    //create semaphores
    //maybe not needed

    //create condition variables
    pthread_condattr_t condatt;
    pthread_condattr_init(&condatt);
    pthread_condattr_setpshared(&condatt, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&sharedmem->cond, &condatt);

    
    //read config file
    if(readConfigFile(argv[1])){
        lprint("Config file read successfully\n");
    }
    else{
        perror("Config file read failed");
        sysclose();
        return 0;
    }

    systemManager();

    sysclose();
    return 0;
}
