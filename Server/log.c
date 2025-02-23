#include <stdio.h> // fprintf
#include <string.h> // strncpy memset strlen strtok
#include <errno.h> // errno
#include <time.h> // time localtime

#include "../Common/packets.h"

#define LOG_OUT stdout
#define LOG_ERR stderr

static void tstamp(FILE * stream);

void log_info(char * msg, int pid) {
    tstamp(LOG_OUT);

    if (pid > 0)
        fprintf(LOG_OUT, " - [CLIENT_%d / INFO] - %s\n", pid, msg);
    else
        fprintf(LOG_OUT, " - [SERVER / INFO] - %s\n", msg);
}

void log_erro(char * msg, int pid) {
    tstamp(LOG_ERR);

    if (pid > 0)
        fprintf(LOG_ERR, " - [CLIENT_%d / ERRO] - %s: %s\n", pid, msg, strerror(errno));
    else
        fprintf(LOG_ERR, " - [SERVER / ERRO] - %s: %s\n", msg, strerror(errno));
}

void log_comm(char * msg, int pid, command_t * cmd) {
    tstamp(LOG_OUT);

    printf(" - [CLIENT_%d / COMM] - %s command received: %s %s\n", pid, msg, msg, cmd->args);
}

void log_resp(int pid, response_t * res) {
    tstamp(LOG_OUT);

    printf(" - [CLIENT_%d / RESP] - %d %s\n", pid, res->code, res->message);
}

static void tstamp(FILE * stream) {
    time_t t = time(NULL);
    struct tm lt = *localtime(&t);

    fprintf(stream, "%d-%02d-%02d %02d:%02d:%02d", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
}