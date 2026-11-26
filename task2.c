#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h> 
#include <time.h>

int main(void){ 
    if (setenv("TZ", "PST8", 1)==-1){
        return 1;
    }

    tzset(); //применяем новый часовой пояс
    time_t now=time(NULL); //получаем текущее время
    struct tm value; 
    char text[80];

    if (now==(time_t)-1||!localtime_r(&now, &value)){
        return 1; 
    }
    if (!strftime(text, sizeof text, "%Y-%m-%d %H:%M:%S %Z", &value)){
        return 1; 
    }
    puts(text);
    return 0; 
}
