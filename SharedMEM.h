typedef struct SharedMEM
{
    //SEARCH: posix mutex across processes
    pthread_mutex_t mutex;
    pthread_mutex_t mutex_log;
    pthread_cond_t cond;
} SharedMEM;