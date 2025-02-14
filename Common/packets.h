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

enum COMMAND_TYPE {
    LIST,   /* List files and directories */
    HELP,   /* Print information about usage */

    RETR,   /* Retrieve file from server */
    STOR,   /* Upload file to server */
    STOU,   /* Upload uniquely-named file to server */
    APPE,   /* Append or create file on server */
    DELE,   /* Delete file on server */

    RMD,    /* Remove directory on server */
    MKD,    /* Make new directory on server */
    PWD,    /* Print working directory */
    CWD,    /* Change working directory */
    CDUP,   /* Change to root directory */

    PASS,
    USER,

    NOOP,
    QUIT
};

#endif