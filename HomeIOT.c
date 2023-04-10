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
#include <string.h>
#include <sys/stat.h>
#include <signal.h>

int QUEUE_SZ;
int N_WORKERS;
int MAX_KEYS;
int MAX_SENSORS;
int MAX_ALERTS;
int shmid;
int fl;
SharedMEM *sharedmem;

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
    //close condition variable
    if(&sharedmem->cond != NULL){
        pthread_cond_destroy(&sharedmem->cond);
        printf("Condition variable closed successfully\n");
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

    exit(0);
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

    pid_t pids[N_WORKERS];
    int num_forks = 0;

    for (int i = 0; i < N_WORKERS; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            worker();
            exit(0);
        } else if (pid > 0) {
            pids[num_forks++] = pid;
        } else {
            // Failed to fork
            perror("Failed to init worker\n");

            for (int j = 0; j < num_forks; j++) {
                kill(pids[j], SIGTERM);
            }
            for(int j = 0; j < 3; j++){
                pthread_cancel(threads[j]);
            }
            //sysclose
            return;
        }
    }

    pid_t pid = fork();
    if(pid==0){
        alertWatcher();
        exit(0);
    }else if(pid==-1){
        perror("Error creating alert watcher");
        for(int i = 0; i < N_WORKERS; i++){
            kill(pids[i], SIGTERM);
        }
        for(int i = 0; i < 3; i++){
            pthread_cancel(threads[i]);
        }
        //fechar o sys
        return;
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
    fl = open("log.txt", O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if(fl == -1){
        perror("Error opening log file");
        return 0;
    }
    
    //create shared memory of SharedMEM struct
    shmid = shmget(IPC_PRIVATE, sizeof(SharedMEM), IPC_CREAT | 0666);
    if (shmid < 0)
    {
        perror("shmget");
        //TODO: fechar o q tenho ate agora (acho q ta tudo)
        close(fl);
        return 0;
    }    
    sharedmem = (SharedMEM *)shmat(shmid, NULL, 0);
    if (sharedmem == (SharedMEM *)-1)
    {
        perror("shmat");
        //TODO: fechar o q tenho ate  (acho q ta tudo)
        shmctl(shmid, IPC_RMID, NULL);
        close(fl);
        return 0;
    }
    lprint("Shared memory created\n");

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
