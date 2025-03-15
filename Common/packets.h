#ifndef PACKETS_H
#define PACKETS_H

#include "defines.h"

/* Command sent by the client */
typedef struct command {
    short int type;
    char args[BUFFER_SIZE];
} command_t;

/* Server's Response to the command */
typedef struct response {
    short int code;
    char message[BUFFER_SIZE];
} response_t;

#ifdef IS_SERVER

void response_set(response_t * res, const int code, const char * message);

#endif

#ifdef IS_CLIENT

int command_set(command_t * cmd);

#endif

#endif