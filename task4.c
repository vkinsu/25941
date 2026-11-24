#define _POSIX_C_SOURCE 200809L 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

struct node{
    struct node *next;
    char text[];
};

int main(void){ 
    struct node *head=NULL, **tail=&head; //начало и место для следующего узла
    char *line=NULL;
    size_t cap=0;
    int status=0;
    
    while (getline(&line, &cap, stdin)>=0&&line[0]!='.'){
        size_t len=strlen(line)+1;
        struct node *p=malloc(sizeof *p+len); 
        if (!p){
            perror("malloc");
            status=1;
            break;
        }
        memcpy(p->text, line, len); //копируем строку в свой узел
        p->next=NULL;
        *tail=p;
        tail=&p->next; //добавляем узел в конец списка
    }

    if (ferror(stdin)){status=1;} //ошибка чтения 
    free(line);
    while (head){ 
        struct node *next=head->next; 
        fputs(head->text, stdout); 
        free(head);
        head=next; 
    }
    return status; 
}
