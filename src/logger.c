#include "../inc/logger.h"

int log_level = INFO_LEVEL;

void log_message(int level, char* format, ...) {
    FILE *f = fopen(LOG_FILE, "a");
    if (f == NULL){
        printf("cant open log file\n");
        return;
    }
    va_list args;
    va_start(args, format);
    if (level >= log_level) {

        time_t current_time;
        char* c_time_string;
        current_time = time(NULL);
        c_time_string = ctime(&current_time);
        char* level_string;
        switch (level) {
            case DEBUG_LEVEL:
                level_string = "DEBUG";
                break;
            case INFO_LEVEL:
                level_string = "INFO";
                break;
            case WARNING_LEVEL:
                level_string = "WARNING";
                break;
            case ERROR_LEVEL:
                level_string = "ERROR";
                break;
        }
         fprintf(f, "%s %s: ", c_time_string, level_string);
         vfprintf(f, format, args);
    }
    
    va_end(args);
    fclose(f);
}