#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>

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
    struct stat st;
    char *mapped;
    struct line_entry table[MAX_LINES];
    int num_lines = 0;
    off_t line_start = 0;
    int line_num;
    int i;
    char input_line[INPUT_BUF];
    int wait_ret;
    int first_request = 1;

    if (argc != 2) {
        fprintf(stderr, "Использование: %s <имя_файла>\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return 1;
    }

    if (st.st_size == 0) {
        printf("Файл пуст.\n");
        close(fd);
        return 0;
    }

    mapped = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    /* Построение таблицы строк */
    for (off_t pos = 0; pos < st.st_size; pos++) {
        if (mapped[pos] == '\n') {
            if (num_lines < MAX_LINES) {
                table[num_lines].offset = line_start;
                table[num_lines].length = (size_t)(pos - line_start + 1);
                num_lines++;
            }
            line_start = pos + 1;
        }
    }

    /* Последняя строка без перевода строки */
    if (line_start < st.st_size && num_lines < MAX_LINES) {
        table[num_lines].offset = line_start;
        table[num_lines].length = (size_t)(st.st_size - line_start);
        num_lines++;
    }

    /* Вывод таблицы */
    for (i = 0; i < num_lines; i++) {
        printf("строка: %d, отступ: %lld длина: %zu\n",
               i + 1, (long long)table[i].offset, table[i].length);
    }

    while (1) {
        printf("Введите номер строки (0 — выход): ");
        fflush(stdout);

        if (first_request) {
            wait_ret = wait_for_input(TIMEOUT_SEC);
            first_request = 0;

            if (wait_ret == 0) {
                printf("\n>> Превышено время ожидания (%d сек). Завершение работы.\n",
                       TIMEOUT_SEC);
                break;
            }
        }

        if (fgets(input_line, sizeof(input_line), stdin) == NULL)
            break;

        if (sscanf(input_line, "%d", &line_num) != 1) {
            printf(">> Ожидается целое число.\n");
            continue;
        }

        if (line_num == 0)
            break;

        if (line_num < 1 || line_num > num_lines) {
            printf(">> Строки с номером %d нет в файле.\n", line_num);
            continue;
        }

        {
            int idx = line_num - 1;
            size_t len = table[idx].length;
            /* Печатаем строку напрямую из отображённой памяти */
            fwrite(mapped + table[idx].offset, 1, len, stdout);
            /* Если строка не заканчивается на '\n', добавляем его */
            if (len > 0 && mapped[table[idx].offset + len - 1] != '\n')
                printf("\n");
        }
    }

    munmap(mapped, st.st_size);
    close(fd);
    return 0;
}