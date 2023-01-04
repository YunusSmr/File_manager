#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define BUFSIZE 100
#define PIPE_NAME "named_pipe"

int main(int argc, char **argv)
{

    char buf[BUFSIZE];
    while (1)
    {
        // Kullanıcıdan komut alınır
        printf(" create <file_name> \n delete <file_name> \n read <file_name> \n write <file_name> <data> \n exit \n Enter command: ");
        fgets(buf, BUFSIZE, stdin);
        buf[strcspn(buf, "\n")] = '\0';
        // Named pipe açılır
        int fd = open(PIPE_NAME, O_WRONLY);
        // Komut server'a gönderilir
        write(fd, buf, strlen(buf) + 1);
        // Named pipe kapatılır
        close(fd);
        // Server'dan cevap alınır
        int bytes_read = read(fd, buf, BUFSIZE);

        buf[bytes_read] = '\0';

        if (strcmp(buf, "error: invalid command") == 0 || strcmp(buf, "error: file not found") == 0 || strcmp(buf, "error: file list is full") == 0)
        {
            printf("Response: %s\n", buf);
            // Geçersiz komut veya dosya bulunamadı durumlarında program sonlandırılır
            continue;
        }
        else if (strcmp(buf, "exit") == 0)
        {
            printf("Response: %s\n", buf);
            break;
            // Başarılı işlemlerde ve 'exit' komutuyla program devam eder
        }
        else
        {
            // 'read' komutuyla server'dan okunan veri ekrana yazdırılır
            int bytes_read = read(fd, buf, BUFSIZE);
            buf[bytes_read] = '\0';
            printf("Response: %s\n", buf);
        }
    }

    return 0;
}
