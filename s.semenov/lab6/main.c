#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>

#define MAX_LINES 10000
#define TIMEOUT_SEC 5 
#define INPUT_BUF   256

struct line_entry {
    off_t  offset;
    size_t length;
};

int wait_for_input(int timeout_sec)
{
    fd_set readfds;
    struct timeval tv;
    int ret;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    tv.tv_sec  = timeout_sec;
    tv.tv_usec = 0;

    while (1) {
        ret = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
        if (ret > 0)  return 1;
        if (ret == 0) return 0;
        if (errno == EINTR) continue;
        return -1;
    }
}

int main(int argc, char *argv[])
{
    int fd;
    struct line_entry table[MAX_LINES];
    int    num_lines  = 0;
    off_t  line_start = 0;
    char   ch;
    int    line_num;
    int    i;
    ssize_t n;
    char  *buf;
    char   input_line[INPUT_BUF];
    int    wait_ret;

    if (argc != 2) {
        fprintf(stderr, "Использование: %s <имя_файла>\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    while (read(fd, &ch, 1) == 1) {
        if (ch == '\n') {
            off_t cur = lseek(fd, 0L, SEEK_CUR);
            if (num_lines < MAX_LINES) {
                table[num_lines].offset = line_start;
                table[num_lines].length = (size_t)(cur - line_start);
                num_lines++;
            }
            line_start = cur;
        }
    }

    {
        off_t file_end = lseek(fd, 0L, SEEK_CUR);
        if (file_end > line_start && num_lines < MAX_LINES) {
            table[num_lines].offset = line_start;
            table[num_lines].length = (size_t)(file_end - line_start);
            num_lines++;
        }
    }

    for (int i = 0; i < num_lines; i++) {
        printf("строка: %d, отступ: %lld длина: %zu\n", i + 1, (long long)table[i].offset, table[i].length);
    }

    int Foccur = 1;
    while (1) {
        printf("Введите номер строки (0 — выход): ");
        fflush(stdout);
        
        if(Foccur) {
            wait_ret = wait_for_input(TIMEOUT_SEC);

            if (wait_ret == 0) {
                printf("\n>> Превышено время ожидания (%d сек). Завершение работы.\n",
                    TIMEOUT_SEC);
                break;
            }
            if (wait_ret < 0) {
                perror("select");
                break;
            }
            Foccur = 0;
        }

        if (fgets(input_line, sizeof(input_line), stdin) == NULL)
            break;

        if (sscanf(input_line, "%d", &line_num) != 1) {
            printf(">> Ожидается целое число.\n\n");
            continue;
        }

        if (line_num == 0)
            break;

        if (line_num < 1 || line_num > num_lines) {
            printf(">> Строки с номером %d нет в файле.\n\n", line_num);
            continue;
        }

        {
            int idx = line_num - 1;
            lseek(fd, table[idx].offset, SEEK_SET);

            buf = (char *)malloc(table[idx].length + 1);
            if (!buf) {
                perror("malloc");
                break;
            }

            n = read(fd, buf, table[idx].length);
            if (n > 0) {
                buf[n] = '\0';
                printf("%d: %s", line_num, buf);
                if (n > 0 && buf[n - 1] != '\n')
                    printf("\n");
            }
            free(buf);
        }
    }

    close(fd);
    return 0;
}