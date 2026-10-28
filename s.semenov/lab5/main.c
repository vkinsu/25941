#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_LINES 10000

struct line_entry {
    off_t  offset;
    size_t length;
};

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <имя_файла>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct line_entry table[MAX_LINES];
    int    num_lines  = 0;
    off_t  line_start = 0;
    char   ch;

    while (read(fd, &ch, 1) == 1) {
        if (ch == '\n') {
            off_t cur = lseek(fd, 0L, 1); 
            
            if (num_lines < MAX_LINES) {
                table[num_lines].offset = line_start;
                table[num_lines].length = (size_t)(cur - line_start);
                num_lines++;
            }
            line_start = cur;
        }
    }

    off_t file_end = lseek(fd, 0L, 1);
    if (file_end > line_start && num_lines < MAX_LINES) {
        table[num_lines].offset = line_start;
        table[num_lines].length = (size_t)(file_end - line_start);
        num_lines++;
    }

    for (int i = 0; i < num_lines; i++) {
        printf("строка: %d, отступ: %lld длина: %zu\n", i + 1, (long long)table[i].offset, table[i].length);
    }

    int line_num;
    while (1) {
        printf("Введите номер строки (0 — выход): ");
        fflush(stdout);
        
        if (scanf("%d", &line_num) != 1)
            break;
            
        if (line_num == 0)
            break;
            
        if (line_num < 1 || line_num > num_lines) {
            printf("Неверный номер строки. Допустимый диапазон: 1-%d\n", num_lines);
            continue;
        }

        int idx = line_num - 1;
        
        lseek(fd, table[idx].offset, SEEK_SET);
        
        char *buf = malloc(table[idx].length + 1);
        if (!buf) { 
            perror("malloc"); 
            break; 
        }
        
        ssize_t n = read(fd, buf, table[idx].length);
        if (n > 0) {
            buf[n] = '\0';
            printf("%s", buf);
            
            if (buf[n - 1] != '\n')
                printf("\n");
        }
        free(buf);
    }

    close(fd);
    return 0;
}