union arguments
{
    char argchar[128];
    int argint;
};

typedef struct UserCommand {
    int console_id;
    char command[128];
    union arguments args[4];
    int num_args;
} UserCommand;

#define USERCOMMANDSIZE sizeof(UserCommand) + 32*sizeof(char)+ 32*4*sizeof(char)