#include <stdio.h> // printf
#include <errno.h> // errno
#include <time.h> // time localtime

#include "tstamp.h"

void tstamp(FILE * stream) {
    time_t t = time(NULL);
    struct tm lt = *localtime(&t);

    fprintf(stream, "%d-%02d-%02d %02d:%02d:%02d", lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec);
}