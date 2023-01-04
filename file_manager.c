#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#define PIPE_NAME "named_pipe"
#define MAX_THREADS 5

// Dosya listesi dizisi
char file_list[10][1024];

// Lock mekanizması
pthread_mutex_t lock;

void *handle_client_request(void *arg)
{
    int *client_pipefd = (int *)arg;

    char buffer[1024]; // İstekleri okumak için buffer

    while (1)
    {
        // Pipe'dan istek oku
        int num_bytes_read = read(*client_pipefd, buffer, 1024);
        if (num_bytes_read == -1)
        {
            perror("Error reading from pipe");
            break;
        }
        close(client_pipefd);
        // Pipe'ı aç
        // client_pipefd = open("named_pipe", O_WRONLY);
        // İstek içeriğini parse et
        char command[1024];
        char file_name[1024];
        char data[1024];
        sscanf(buffer, "%s %s %s", command, file_name, data);

        // Lock mekanizmasını kullanarak dosya listesi dizisini güncelle
        // pthread_mutex_lock(&lock);
        if (strcmp(command, "create") == 0)
        {
            printf("command: %s\nfileName: %s \ndata : %s \n ", command, file_name, data);

            // Boş olan bir sırayı bul
            int index = -1;
            for (int i = 0; i < 10; i++)
            {
                if (strlen(file_list[i]) == 0)
                {
                    index = i;
                    break;
                }
            }

            if (index == -1)
            {
                // Boş sıra yok
                sprintf(buffer, "Error: file list is full");
            }
            else
            {
                // Dosya ismini dosya listesi dizisine ekle
                strcpy(file_list[index], file_name);

                // Dosyayı oluştur
                int fd = open(file_name, O_CREAT | O_WRONLY, 0666);
                close(fd);

                // Cevap oluştur
                sprintf(buffer, "Successfully created file %s", file_name);
                // write(*client_pipefd, buffer, strlen(buffer) + 1);
            }
        }
        else if (strcmp(command, "delete") == 0)
        {
            sscanf(buffer, "%s %s %s", command, file_name, data);
            // Dosya listesi dizisinde dosya ismini arama
            int index = -1;
            for (int i = 0; i < 10; i++)
            {
                if (strcmp(file_list[i], file_name) == 0)
                {
                    index = i;
                    break;
                }
            }

            if (index == -1)
            {
                // Dosya ismi bulunamadı
                sprintf(buffer, "Error: file not found");
            }
            else
            {
                // Dosya ismini dosya listesi dizisinden sil
                strcpy(file_list[index], "");

                // Dosyayı sil
                unlink(file_name);

                // Cevap oluştur
                sprintf(buffer, "Successfully deleted file %s", file_name);
            }
        }
        else if (strcmp(command, "read") == 0)
        {
            // Dosya listesi dizisinde dosya ismini arama
            int index = -1;
            for (int i = 0; i < 10; i++)
            {
                if (strcmp(file_list[i], file_name) == 0)
                {
                    index = i;
                    break;
                }
            }
            if (index == -1)
            {
                // Dosya ismi bulunamadı
                sprintf(buffer, "Error: file not found");
            }
            else
            {
                // Dosyayı oku
                int fd = open(file_name, O_RDONLY);
                if (fd == -1)
                {
                    sprintf(buffer, "Error: failed to open file");
                }
                else
                {
                    // Dosyadan okunacak veri miktarını hesapla
                    int num_bytes_to_read = data;
                    if (num_bytes_to_read > 1024)
                    {
                        num_bytes_to_read = 1024;
                    }

                    // Veriyi oku
                    int num_bytes_read = read(fd, buffer, num_bytes_to_read);
                    if (num_bytes_read == -1)
                    {
                        perror("Error reading from file");
                        sprintf(buffer, "Error: failed to read file");
                    }
                    else
                    {
                        // Okunan veriyi null-terminate et
                        buffer[num_bytes_read] = '\0';
                    }

                    close(fd);
                }
            }
        }
        else if (strcmp(command, "write") == 0)
        {

            // Dosya listesi dizisinde dosya ismini arama
            int index = -1;
            for (int i = 0; i < 10; i++)
            {
                if (strcmp(file_list[i], file_name) == 0)
                {
                    index = i;
                    printf("index: %d\n", index);
                    break;
                }
            }
            if (index == -1)
            {
                // Dosya ismi bulunamadı
                sprintf(buffer, "Error: file not found");
            }
            else
            {
                // Dosyayı aç
                int fd = fopen(file_name, "a");
                if (fd == -1)
                {
                    sprintf(buffer, "Error: failed to open file");
                }
                else
                {

                    // Veriyi dosyaya yaz
                    fprintf(fd, "%s\n", data);
                    // write(fd, file_name, data);

                    // Cevap oluştur
                    sprintf(buffer, "Successfully wrote %d bytes to file %s", file_name);

                    close(fd);
                }
            }
        }
        else if (strcmp(command, "exit") == 0)
        {
            // Thread'i bitir
            break;
        }
        else
        {
            // Geçersiz komut
            sprintf(buffer, "Error: invalid command");
            // Cevabı pipe'a yaz
            // write(*client_pipefd, buffer, 1024);
        }

        // Lock mekanizmasını serbest bırak
        // pthread_mutex_unlock(&lock);
    }
    close(client_pipefd);

    return NULL;
}

int main()
{
    // Named pipe'ı oluştur
    mkfifo("named_pipe", 0666);

    // Lock mekanizmasını oluştur
    pthread_mutex_init(&lock, NULL);

    // Dosya listesi dizisini sıfırla
    memset(file_list, 0, sizeof(file_list));

    // Threadleri tutacak dizi
    pthread_t threads[MAX_THREADS];
    int thread_count = 0;

    while (1)
    {
        // Pipe'ı aç
        int named_pipe_fd = open("named_pipe", O_RDONLY);
        if (named_pipe_fd == -1)
        {
            perror("Error opening named pipe");
            return 1;
        }

        // Thread oluştur
        pthread_create(&threads[thread_count], NULL, handle_client_request, (void *)&named_pipe_fd);
        thread_count++;

        // Maksimum thread sayısına ulaşıldıysa, bekle
        if (thread_count == MAX_THREADS)
        {
            thread_count = 0;
        }
        for (int i = 0; i < MAX_THREADS; i++)
        {
            pthread_join(threads[i], NULL);
        }
    }

    // Lock mekanizmasını temizle
    pthread_mutex_destroy(&lock);

    return 0;
}