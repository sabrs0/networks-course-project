#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#define DEBUG_LEVEL 0
#define INFO_LEVEL 1
#define WARNING_LEVEL 2
#define ERROR_LEVEL 3
#define LOG_FILE "log.txt"
#define LOG_BUF 512

void log_message(int level, char* format, ...) ;