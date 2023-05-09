#include <sys/types.h>

#define MSG_KEY 1234

typedef struct MQMessage{
    long mtype;
    char mtext[1024];
} MQMessage;