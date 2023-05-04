union arguments
{
    char *argchar;
    int argint;
};

typedef struct UserCommand {
    int console_id;
    char *command;
    union arguments *args;
} UserCommand;

#define USERCOMMANDSIZE sizeof(UserCommand) + 64*sizeof(char)