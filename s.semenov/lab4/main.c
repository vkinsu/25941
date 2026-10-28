#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEN 1024

struct node {
    char *str;
    struct node *next;
};

void remove_escapes(char *s) {
    char *src = s, *dst = s;
    while (*src) {
        if (*src == '\033' && *(src + 1) == '[') {
            src += 2;
            while (*src && *src >= 0x20 && *src <= 0x2F) {
                src++;
            }
            if (*src && *src >= 0x40 && *src <= 0x7E) {
                src++;
            }
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

int main() {
    struct node *head = NULL;
    struct node *tail = NULL;
    char buf[MAX_LEN];

    // Читаем строки с помощью fgets
    while (fgets(buf, MAX_LEN, stdin) != NULL) {
        if (buf[0] == '.')
            break;
            
        remove_escapes(buf);
        
        struct node *n = (struct node *)malloc(sizeof(struct node));
        if (n == NULL) {
            perror("malloc");
            exit(1);
        }
        
        n->str = (char *)malloc(strlen(buf) + 1);
        if (n->str == NULL) {
            perror("malloc");
            exit(1);
        }
        
        strcpy(n->str, buf);
        n->next = NULL;
        
        // Добавляем узел в конец списка
        if (head == NULL)
            head = n;
        else
            tail->next = n;
        tail = n;
    }

    struct node *cur = head;
    while (cur != NULL) {
        printf("%s", cur->str);
        cur = cur->next;
    }

    cur = head;
    while (cur != NULL) {
        struct node *tmp = cur;
        cur = cur->next;
        free(tmp->str);
        free(tmp);
    }

    return 0;
}