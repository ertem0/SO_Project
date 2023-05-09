#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

union arguments {
    char *argchar;
    int argint;
};

typedef struct UserCommand {
    int console_id;
    char command[128];
    union arguments *args;
    int num_args;
} UserCommand;

int main() {
    const char *fifo_name = "/tmp/myfifo";

    if (mkfifo(fifo_name, 0666) == -1) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Child process
        int fd = open(fifo_name, O_RDONLY);
        if (fd == -1) {
            perror("open (child)");
            exit(EXIT_FAILURE);
        }

        UserCommand received;
        read(fd, &received, sizeof(received));
        close(fd);

        printf("Child process: Received console_id: %d, command: %s, num_args: %d\n",
               received.console_id, received.command, received.num_args);

        exit(EXIT_SUCCESS);
    } else { // Parent process
        int fd = open(fifo_name, O_WRONLY);
        if (fd == -1) {
            perror("open (parent)");
            exit(EXIT_FAILURE);
        }

        UserCommand to_send;
        to_send.console_id = 1;
        strncpy(to_send.command, "example_command", sizeof(to_send.command) - 1);
        to_send.command[sizeof(to_send.command) - 1] = '\0';
        to_send.args = NULL;
        to_send.num_args = 0;

        write(fd, &to_send, sizeof(to_send));
        close(fd);

        wait(NULL); // Wait for child process to finish
        unlink(fifo_name); // Remove the named pipe file
        exit(EXIT_SUCCESS);
    }

    return 0;
}
