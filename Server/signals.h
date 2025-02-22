#ifndef SIGNALS_H
#define SIGNALS_H

void sigint_handler(int signo);
void sigchld_handler(int signo);

#endif