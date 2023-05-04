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
#include "InternalQueue.h"

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
        printf("Sensor pipe mutex closed successfully\n");
    }
    if(&sharedmem->mutex_userc_pipe != NULL){
        pthread_mutex_destroy(&sharedmem->mutex_userc_pipe);
        printf("User console pipe mutex closed successfully\n");
    }
    if(&sharedmem->mutex_unamed_pipe != NULL){
        pthread_mutex_destroy(&sharedmem->mutex_unamed_pipe);
        printf("Unamed pipe mutex closed successfully\n");
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

    free_queue(iq);

    exit(0);
}

void ctrlc_handler(){
    lprint("Received SIGINT. Closing program...\n");
    //close threads
    for(int i = 0; i < 3; i++){
        pthread_cancel(threads[i]);
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

    char buf[100];

    //UGLY: perguntar ao stor se posso terminar o worker ao meter END na MSGUEUE
    while(is_running){
        pthread_mutex_lock(&sharedmem->mutex_wait_worker_ready);
        pthread_mutex_lock(&sharedmem->mutex_worker_ready);
        sharedmem->worker_ready[id] = 1;
        pthread_mutex_unlock(&sharedmem->mutex_worker_ready);
        pthread_cond_signal(&sharedmem->cond_wait_worker_ready);
        pthread_mutex_unlock(&sharedmem->mutex_wait_worker_ready);

        read(upipefd[id][0], buf, 100);

        is_working = 1;
        
        //wait for signal
        printf("Worker %d received %s\n", getpid(), buf);
        //TODO: meter um while para prevenir o spam de ctrlc
        int x = sleep(30);
        if(x > 0){
            sleep(x);
        }

        is_working = 0;
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
    while (1)
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
        }
        
        pthread_mutex_lock(&sharedmem->mutex_iq);
        printf("Dispacher %ld waiting for iq\n", my_id);
        while(is_empty(iq))
            pthread_cond_wait(&sharedmem->cond_iq, &sharedmem->mutex_iq);
        data = remove_message(iq);
        if(data == NULL){
            printf("Dispacher %ld received NULL\n", my_id);
            pthread_mutex_unlock(&sharedmem->mutex_iq);
            continue;
        }
        pthread_mutex_unlock(&sharedmem->mutex_iq);
        
        printf("Dispacher %ld received %s\n", my_id, data);
        
        printf("Dispacher %ld sending to worker %d\n", my_id, wpids[found]);
        //UGLY: perguntar ao stor sobre o tamanho do buffer
        write(upipefd[found][1], data, sizeof(data));
        pthread_mutex_lock(&sharedmem->mutex_worker_ready);
        sharedmem->worker_ready[found] = 0;
        pthread_mutex_unlock(&sharedmem->mutex_worker_ready);


        free(data);
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
    pthread_mutex_lock(&sharedmem->mutex_userc_pipe);
    if ((console_fd = open(CONSOLE_FIFO, O_RDONLY)) < 0) {
        perror("Cannot open pipe for reading: ");
        pthread_mutex_unlock(&sharedmem->mutex_userc_pipe);
    }
    pthread_mutex_unlock(&sharedmem->mutex_userc_pipe);
    printf("console_fifo opened\n");

    UserCommand uc;
    ssize_t bytes_read;
    //read from console
    while(1){
        pthread_mutex_lock(&sharedmem->mutex_userc_pipe);
        bytes_read = read(console_fd, &uc, USERCOMMANDSIZE);
        Data data;
        data.user_command = &uc;
        pthread_mutex_unlock(&sharedmem->mutex_userc_pipe);
        if(bytes_read < 0){
            perror("Error reading from console: ");
        }
        else if(bytes_read == 0){
            //printf("Console closed\n");
            //break;
        }
        else {
            // buffer[bytes_read] = '\0';
            printf("Read from console: %s\n", uc.command);
            //add to queue
            pthread_mutex_lock(&sharedmem->mutex_iq);
            if(add_message(iq, &data, 1) == -1){
                lprint("Queue is full, message (%s) not added\n", uc.command);
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

    //close pipe
    pthread_mutex_lock(&sharedmem->mutex_userc_pipe);
    close(console_fd);
    unlink(CONSOLE_FIFO);
    pthread_mutex_unlock(&sharedmem->mutex_userc_pipe);

    lprint("Console Reader %ld finished\n", my_id);
    pthread_exit(NULL);
}

void *sensorReader(void *arg){
    long my_id = (long)arg;
    lprint("Sensor Reader %ld started\n", my_id);

    if ((sensor_fd = open(SENSOR_FIFO, O_RDONLY)) < 0) {
        perror("Cannot open pipe for reading: ");
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
    unsigned long sizeof_keylist = sizeof(KeyList) + sizeof(KeyNode) * MAX_KEYS;
    //create shared memory of SharedMEM struct
    shmid = shmget(IPC_PRIVATE, sizeof(SharedMEM) + sizeof(int)*N_WORKERS + sizeof_keylist, IPC_CREAT | 0666);
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
    sharedmem->worker_ready = (int*)((void*)sharedmem) + sizeof(SharedMEM);
    for(int i = 0; i < N_WORKERS; i++){
        sharedmem->worker_ready[i] = 0;
    }
    sharedmem->keylist = (KeyList*)((void*)sharedmem->worker_ready) + sizeof(int)*N_WORKERS;
    sharedmem->keylist->size = 0;
    sharedmem->keylist->max_size = MAX_KEYS;
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
    if(pthread_mutex_init(&sharedmem->mutex_userc_pipe, &att) != 0){
        printf("Failed to initialize alert mutex.\n");
        sysclose();
        return 1;
    }
    if(pthread_mutex_init(&sharedmem->mutex_unamed_pipe, &att) != 0){
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


//diferneciar o q veio do console e o que veio do sensor