#define _POSIX_C_SOURCE 200809L

#include <stdio.h> 
#include <stdlib.h>
#include <fcntl.h> 
#include <unistd.h> 

struct line{
    off_t start, len;
};

int main(int argc, char **argv){ 
    //проверка пути и чтение 
    if (argc!=2){fprintf(stderr, "usage: %s FILE\n", argv[0]);return 1;}
    int fd=open(argv[1], O_RDONLY); 
    if (fd<0){perror("open");return 1;} 

    struct line *table=NULL; 
    size_t count=0, cap=0; 
    off_t start=0, pos=0; 
    int status=1;
    for (;;){
        char ch; 
        ssize_t n=read(fd, &ch, 1); 
        if (n<0){perror("read");goto done;} //ошибка индексации

        pos+=n; 
        if ((!n||ch=='\n')&&pos>start){ 
            if (count==cap){ 
                cap=cap?cap*2:32; 
                struct line *tmp=realloc(table, cap*sizeof *table); 
                if (!tmp){ //при ошибке сохраняем старую таблицу
                    perror("realloc");
                    goto done;
                }
                table=tmp; 
            } 
            table[count++]=(struct line){start, pos-start}; 
            start=pos; 
        } 
        if (!n){
            break;
        }
    } 

    for (;;){ 
        char input[80], *end; 
        printf("line (0 exits): ");
        fflush(stdout); 
        char *answer=fgets(input, sizeof input, stdin); 

        if (!answer){
            if (ferror(stdin)){
                goto done;
            }
            break;
        } 
        long no=strtol(input, &end, 10); 
        if (end==input||(*end&&*end!='\n')||no<0||(unsigned long)no>count){
            puts("invalid line");
            continue;
        } 
        if (!no){
            break;
        }
        struct line row=table[no-1]; 
        if (lseek(fd, row.start, SEEK_SET)<0){
            goto done; 
        }
        while (row.len>0){
            char buf[4096]; 
            size_t want=
                row.len>(off_t)sizeof buf?sizeof buf:(size_t)row.len; 
            ssize_t n=read(fd, buf, want); 
            if (n<=0||fwrite(buf, 1, n, stdout)!=(size_t)n){
                goto done;
            }
            row.len-=n; 
        } 
    } 
    status=0; 
done: //зачем--
    free(table); 
    close(fd); 
    return status; 
}
