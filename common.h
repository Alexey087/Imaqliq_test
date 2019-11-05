#ifndef _COMMON_H_
#define _COMMON_H_

#include <stddef.h>
#include <unistd.h>

#define BUF_SIZE 8192
#define MAXPATH 512

static const int g_buf_size = 100;
char g_time[100];

char g_mes[MAXPATH];
size_t g_size_mes;

void report_write(const char *mes);
void failure_write(const char *mes);
char *getTime();

#endif // _COMMON_H_