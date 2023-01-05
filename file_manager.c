#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

// variables
#define PIPE_NAME "/tmp/named_pipe"
#define MAX_THREADS 5
#define MAX_FILES 10
#define BUFSIZE 1024

// Lock mekanizması
pthread_mutex_t lock;

char response[BUFSIZE];

// functions signuture
void open_mPipe(char *pipeName);
int _read(char *pipeName, char *buffer);
int _write(char *pipeName, char *msg);
void get_commands();
void *handle_client_request(void *arg);
void create_request(char *file_name, char *pipeName);
void delete_request(char *file_name, char *pipeName);
void read_request(char *file_name, char *pipeName);
void write_request(char *file_name, char *data, char *pipeName);
int index_file(char *fileName);
int empty_index();
int isExist(char *fileName);

// Gerekli yerlerde struct, fonksiyonlar, thread, lock mekanizmaları kullanılarak yapılacaktır.
typedef struct
{
    pthread_t thread;
    char *name;
    int status;
} Pipe;

// pipeList
Pipe pipeList[MAX_THREADS];

// file_list
char *file_list[MAX_FILES];

// open main pipe with mkfifo
void open_mPipe(char *pipeName)
{
    // if exist unlink before create
    unlink(pipeName);
    if (mkfifo(pipeName, 0666) < 0)
    {
        perror("mkfifo");
        exit(1);
    }
}

// Find an empty index in the file_list
int empty_index()
{

    for (int i = 0; i < MAX_FILES; i++)
    {
        if (file_list[i] == NULL)
        {
            return i;
        }
    }
    // If no empty index , return -1
    return -1;
}

// returns index of filename
int index_file(char *fileName)
{
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (file_list[i] && strcmp(file_list[i], fileName) == 0)
        {
            return i;
        }
    }

    return -1;
}

// check an file exist or not, if true return 1
int isExist(char *fileName)
{
    for (int i = 0; i < MAX_FILES; i++)
    {
        if (file_list[i] != NULL && strcmp(file_list[i], fileName) == 0)
        {
            printf("file has found\n");
            return 1;
        }
    }
    return 0;
}

// handles client request
void *handle_client_request(void *index)
{

    int *i = (int *)index;
    char *pipeName = pipeList[*i].name;

    printf("%s connected to manager \n", pipeName);
    char file_name[100];

    char data[1024];
    char command[1024];
    char buffer[BUFSIZE];

    open_mPipe(pipeName);

    while (1)
    {
        _read(pipeName, buffer);

        sscanf(buffer, "%s %s %s", command, file_name, data);
        printf("\n--->Request: %s\n\ncommand: %s\nfileName: %s \ndata : %s\n", buffer, command, file_name, data);
        if (strcmp(command, "create") == 0)
        {
            pthread_mutex_lock(&lock);
            create_request(file_name, pipeName);
            pthread_mutex_unlock(&lock);
        }
        else if (strcmp(command, "delete") == 0)
        {
            pthread_mutex_lock(&lock);
            delete_request(file_name, pipeName);
            pthread_mutex_unlock(&lock);
        }
        else if (strcmp(command, "read") == 0)
        {
            pthread_mutex_lock(&lock);

            read_request(file_name, pipeName);
            pthread_mutex_unlock(&lock);
        }
        else if (strcmp(command, "write") == 0)
        {
            pthread_mutex_lock(&lock);
            write_request(file_name, data, pipeName);
            pthread_mutex_unlock(&lock);
        }
        else if (strcmp(command, "exit") == 0)
        {
            printf("%s disconnected from manager \n", pipeName);
            pipeList[*i].status = 0;
            break;
        }
        else
        {
            _write(pipeName, "Error: invalid command");
            printf("Error--invalid command: %s\n", command);
        }
    }
    free(index);
    return NULL;
}

// handles create request
void create_request(char *file_name, char *pipeName)
{
    if (strlen(file_name) <= 0)
    {
        _write(pipeName, "write an exceptable file name!");
        return;
    }

    if (isExist(file_name))
    {
        _write(pipeName, "file is already exist");
        return;
    }

    int index = empty_index();

    if (index == -1)
    {
        _write(pipeName, "Error: file list is full");
        return;
    }

    file_list[index] = (char *)malloc(1024);
    memcpy(file_list[index], file_name, strlen(file_name) + 1);

    FILE *fp = fopen(file_name, "w");
    if (fp == NULL)
    {
        perror("fopen");
        exit(1);
    }
    fclose(fp);
    sprintf(response, "Successfully created file %s", file_name);
    _write(pipeName, response);
}

// handles delete request
void delete_request(char *file_name, char *pipeName)
{
    int index = index_file(file_name);
    if (index == -1)
    {
        _write(pipeName, "Error: file not found");
        return;
    }

    free(file_list[index]);
    file_list[index] = NULL;

    unlink(file_name);
    sprintf(response, "Successfully deleted file %s", file_name);
    _write(pipeName, response);
}

// handles read request
void read_request(char *file_name, char *pipeName)
{
    if (!isExist(file_name))
    {
        _write(pipeName, "Error: file not found");
        return;
    }
    FILE *fd;
    fd = fopen(file_name, "r");
    if (!fd)
    {
        _write(pipeName, "Error: failed to open file");
        return;
    }

    char data[256];
    int index = 0;
    char c;
    while ((c = fgetc(fd)) != EOF)
    {
        data[index++] = c;
    }
    data[index] = '\0';
    sprintf(response, "readed data : %s", data);
    _write(pipeName, response);
}

// handles write request
void write_request(char *file_name, char *data, char *pipeName)
{

    if (!isExist(file_name))
    {
        _write(pipeName, "Error: file not found");
        return;
    }
    FILE *fd;
    fd = fopen(file_name, "a");
    if (!fd)
    {
        _write(pipeName, "Error: failed to open file");
        return;
    }

    fprintf(fd, "%s", data);
    fprintf(fd, "%s", "\n");


    sprintf(response, "Successfully wrote to file %s", file_name);
    _write(pipeName,response);
    fclose(fd);
}

// writes to pipeName and close it
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

// writes from pipeName and close it
int _read(char *pipeName, char *buffer)
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

// get commands from clients
void get_commands()
{

    while (1)
    {
        char command[BUFSIZE];
        _read(PIPE_NAME, command);
        int connectionStatus = 0;
        for (size_t i = 0; i < MAX_THREADS; i++)
        {
            if (!pipeList[i].status)
            {
                _write(PIPE_NAME, pipeList[i].name);
                int *param = malloc(sizeof(int));
                *param = i;
                pthread_create(&pipeList[i].thread, NULL, handle_client_request, param);
                pipeList[i].status = 1;
                connectionStatus = 1;
                break;
            }
        }

        if (!connectionStatus)
        {
            printf("too much client\n");
            _write(PIPE_NAME, "too much client");
        }
    }
}

int main()
{
    pthread_mutex_init(&lock, NULL);

    for (size_t i = 0; i < MAX_FILES; i++)
    {
        file_list[i] = NULL;
    }

    // open main pipe
    open_mPipe(PIPE_NAME);

    for (int i = 0; i < MAX_THREADS; i++)
    {
        pipeList[i].status = 0;
        char *name = malloc(25);
        sprintf(name, "%d.pipe", i);
        printf("%s has created \n", name);
        pipeList[i].name = name;
    }

    // waits for commands from clients
    get_commands();

    pthread_mutex_destroy(&lock);
    return 0;
}