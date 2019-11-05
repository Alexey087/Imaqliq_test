#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>

#include "common.h"

void report_write(const char *mes)
{    
    fprintf(stderr, "ok %s: %s\n", getTime(), mes);    
}

void failure_write(const char *mes)
{  
    fprintf(stderr, "err %s: %s\n", getTime(), mes);
}

char *getTime()
{
    time_t now = time(NULL);
    struct tm *ptr = localtime(&now);
    strftime(g_time, g_buf_size, "%Y.%m.%d %H:%M:%S", ptr);  
    
    return g_time;
}