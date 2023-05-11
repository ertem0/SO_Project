#include <pthread.h>
#include "Lists.h"

typedef struct SharedMEM
{
    //SEARCH: posix mutex across processes
    pthread_mutex_t mutex;
    pthread_mutex_t mutex_log;
    pthread_mutex_t mutex_worker_ready;
    pthread_mutex_t mutex_wait_worker_ready;
    pthread_mutex_t mutex_iq;
    pthread_mutex_t mutex_keylist;
    pthread_mutex_t mutex_sensorlist;
    pthread_mutex_t mutex_alertlist;
    pthread_cond_t cond;
    pthread_cond_t cond_wait_worker_ready;
    pthread_cond_t cond_iq;
    pthread_cond_t cond_alertlist;
    int *worker_ready;
    KeyList keylist;
    SensorList sensorlist;
    AlertList alertlist;
    //ugly: perguntar ao stor sobre onde fica  a mq
} SharedMEM;

typedef struct Worker{
    pid_t pid;
    int is_ready;
} Worker;