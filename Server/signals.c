#include <stdlib.h>
#include <sys/wait.h> // waitpid
#include <errno.h> // errno

#include "../Common/defines.h"
#include "signals.h"

extern volatile int isClosing;
extern volatile int nChildren;

void sigint_handler(int signo) { 
    isClosing = TRUE; 
}

void sigchld_handler(int signo) {
    // Taken from https://beej.us/guide/bgnet/
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}