#include <stdio.h> // printf perror fgets (stdin stderr)
#include <unistd.h> // access
#include <string.h> // strncpy memset strlen strtok
#include <sys/types.h>
#include <ctype.h> // toupper

#include "string_helpers.h"
#include "command_types.h"
#include "packets.h"

void response_set(response_t * res, const int code, const char * message) {
    res->code = code;
    strncpy(res->message, message, BUFFER_SIZE-1);
    res->message[BUFFER_SIZE-1] = '\0';
}

int command_set(command_t * cmd) {
    char buffer[BUFFER_SIZE + 4]; // +4 to account for type
    char cmdTypeRaw[5];
    char *token;

    memset(&buffer, 0, sizeof(buffer));
    memset(&cmdTypeRaw, 0, sizeof(cmdTypeRaw));

    // GET INPUT

    fgets(buffer, sizeof(buffer), stdin);

    // ... Type
    token = strtok(buffer, " ");
    if (token != NULL) {
        trim(token);
        strncpy(cmdTypeRaw, token, sizeof(cmdTypeRaw));
    } else {
        return -1;
    }

    if(strlen(cmdTypeRaw) > 4) {
        return -1;
    }

    for (int i = 0; i < sizeof(cmdTypeRaw); ++i) {
        cmdTypeRaw[i] = toupper(cmdTypeRaw[i]);
    }

    // ... Args
    token = strtok(NULL, "\n");
    if (token != NULL) {
        trim(token);
        strncpy(cmd->args, token, sizeof(cmd->args));
    } else {
        cmd->args[0] = '\0';
    }



    // PARSE COMMAND TYPE

    if (strncmp(cmdTypeRaw, "LIST", 4) == 0) {
        cmd->type = LIST;
    } else if (strncmp(cmdTypeRaw, "HELP", 4) == 0) {
        cmd->type = HELP;
        cmd->args[0] = '\0';
    } else if (strncmp(cmdTypeRaw, "RETR", 4) == 0) {
        cmd->type = RETR;
    } else if (strncmp(cmdTypeRaw, "STOR", 4) == 0) {
        cmd->type = STOR;

        if(access(cmd->args, F_OK) < 0) {
            fprintf(stderr, "<ERR> ");
            perror("access()");

            return -1;
        }
    } else if (strncmp(cmdTypeRaw, "STOU", 4) == 0) {
        cmd->type = STOU;
    } else if (strncmp(cmdTypeRaw, "APPE", 4) == 0) {
        cmd->type = APPE;
    } else if (strncmp(cmdTypeRaw, "DELE", 4) == 0) {
        cmd->type = DELE;
    } else if (strncmp(cmdTypeRaw, "RMD", 3) == 0) {
        cmd->type = RMD;
    } else if (strncmp(cmdTypeRaw, "MKD", 3) == 0) {
        cmd->type = MKD;
    } else if (strncmp(cmdTypeRaw, "PWD", 3) == 0) {
        cmd->type = PWD;
        cmd->args[0] = '\0';
    } else if (strncmp(cmdTypeRaw, "CWD", 3) == 0) {
        cmd->type = CWD;
    } else if (strncmp(cmdTypeRaw, "CDUP", 4) == 0) {
        cmd->type = CDUP;
        cmd->args[0] = '\0';
    } else if (strncmp(cmdTypeRaw, "PASS", 4) == 0) {
        cmd->type = PASS;
    } else if (strncmp(cmdTypeRaw, "USER", 4) == 0) {
        cmd->type = USER;
    } else if (strncmp(cmdTypeRaw, "NOOP", 4) == 0) {
        cmd->type = NOOP;
        cmd->args[0] = '\0';
    } else if (strncmp(cmdTypeRaw, "QUIT", 4) == 0) {
        cmd->type = QUIT;
        cmd->args[0] = '\0';
    } else if (strncmp(cmdTypeRaw, "PASV", 4) == 0) {
        cmd->type = PASV;
        cmd->args[0] = '\0';
    } else if (strncmp(cmdTypeRaw, "PORT", 4) == 0) {
        cmd->type = PORT;
    } else {
        return -1;
    }

    return 0;
}