#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PIPE_NAME "/tmp/named_pipe"
#define BUFSIZE 1024

char buffer[BUFSIZE];

int _write(char *pipeName, char *msg)
{
    int fd = open(pipeName, O_WRONLY);
    if (fd < 0)
    {
        perror("couldnt open pipe");
        return -1;
    }

    if (write(fd, msg, strlen(msg) + 1) < 0)
    {
        perror("couldnt write buffer from pipe");
        return -1;
    }
    close(fd);
    return 0;
}

int _read(char *pipeName)
{
    int fd = open(pipeName, O_RDONLY);
    if (fd < 0)
    {
        perror("couldnt open pipe");
        return -1;
    }
    size_t len = read(fd, buffer, BUFSIZE);
    if (len < 0)
    {
        perror("couldnt read to pipe");
        return -1;
    }
    close(fd);

    return len;
}

int main()
{
    // connect to main pipe
    _write(PIPE_NAME, "connect to main pipe");

    size_t pipeNameLen = _read(PIPE_NAME);
    char pipeName[pipeNameLen];

    strcpy(pipeName, buffer);

    if (strcmp("too much client", pipeName) == 0)
    {
        printf("wait other client until done...try again ");
        return 0;
    }

    // get commands and write to pipe
    while (1)
    {
        printf(" create <file_name> \n delete <file_name> \n read <file_name> \n write <file_name> <data> \n exit \n Enter command: ");
        fgets(buffer, BUFSIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        _write(pipeName, buffer);
        if (strcmp(buffer, "exit") == 0)
        {
            break;
        }
        else
        {
            // prints response from manager
            _read(pipeName);
            printf("\n---> Response: %s\n\n", buffer);
        }
    }

    return 0;
}