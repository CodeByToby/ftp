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

#endif