#define _GNU_SOURCE 

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h> 
#include <errno.h> 
static volatile sig_atomic_t expired; 

static void timeout(int sig){
    (void)sig;
    expired=1;
}

static sigset_t waiting_mask;

static int setup_mask(void){
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
    if (sigprocmask(SIG_BLOCK, &set, &waiting_mask)<0){
        return -1;
    }
    sigdelset(&waiting_mask, SIGALRM);
    return 0;
}

static ssize_t next_byte(int fd, char *ch){
    while (!expired){
        struct pollfd input={.fd=fd,.events=POLLIN};
        int ready=ppoll(&input, 1, NULL, &waiting_mask);
        if (ready>0){
            return read(fd, ch, 1);
        }
        if (ready<0&&errno!=EINTR){
            return -1;
        }
    }
    errno=EINTR;
    return -1;
}

static void stop_timer(void){
    alarm(0); //останавливаем обратный отсчет
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGALRM); //проверяем только сигнал таймера
    struct timespec zero={0};
    while (sigtimedwait(&set, NULL, &zero)<0&&errno==EINTR){
    }
}

struct line {
    off_t start, len;
};

int main(int argc, char **argv){
    //прповерка пути и чтение
    if (argc!=2){
        fprintf(stderr, "usage: %s FILE\n", argv[0]);
        return 1;
    }
    int fd=open(argv[1], O_RDONLY);
    if (fd<0){
        perror("open");
        return 1;
    }
    struct line *table=NULL;
    size_t count=0, cap=0;
    off_t start=0, pos=0;
    int status=1;
    for (;;){
        char ch;
        ssize_t n=read(fd, &ch, 1);
        if (n<0){
            perror("read");
            goto done;
        }
        pos+=n;
        if ((!n||ch=='\n')&&pos>start){
            if (count==cap){
                cap=cap?cap*2:32;
                struct line *tmp=realloc(table, cap*sizeof *table);
                if (!tmp){
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
    struct sigaction sa={0};
    sa.sa_handler=timeout;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGALRM, &sa, NULL)<0||setup_mask()<0){
        goto done;
    }
    for (;;){
        char input[80], *end;
        printf("line (0 exits): ");
        fflush(stdout);
        alarm(5);
        size_t used=0;
        int too_long=0;
        ssize_t got;
        char ch;
        while ((got=next_byte(0, &ch))>0&&ch!='\n'){
            if (used+1<sizeof input){
                input[used++]=ch;
            } else {
                too_long=1;
            }
        }
        input[used]=0;
        stop_timer();
        if (expired){
            char buf[4096];
            if (lseek(fd, 0, SEEK_SET)<0){
                goto done;
            }
            ssize_t n;
            while ((n=read(fd, buf, sizeof buf))>0){
                if (fwrite(buf, 1, n, stdout)!=(size_t)n){
                    goto done;
                }
            }
            if (n<0){
                goto done;
            }
            break;
        }
        if (got<0){
            goto done;
        }
        if (!got&&!used){
            break;
        }
        if (too_long){
            puts("invalid line");
            continue;
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
done:

    free(table);
    close(fd);
    return status;
}
