#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  /* strncpy() */

#include "../Common/packets.h"
#include "../Common/defines.h"
#include "handlers.h"

int ftp_list(response_t * res, const command_t * cmd, const user_session_t * session) {
    if (session->state != LOGGED_IN) {
        res->code = 530;
        strncpy(res->message, "User not logged in", BUFFER_SIZE);
    } else {
        res->code = 502;
        strncpy(res->message, "Command not yet implemented", BUFFER_SIZE);
    }

    return 0;
}

int ftp_noop(response_t * res) {
    res->code = 200;
    strncpy(res->message, "Command okay", BUFFER_SIZE);

    return 0;
}

int ftp_pass(response_t * res, const command_t * cmd, user_session_t * session) {
    //TODO: Handle login
    
    // Session
    session->state = LOGGED_IN;
    strncpy(session->username, "user", BUFFER_SIZE);
    strncpy(session->path, "home/user", BUFFER_SIZE);

    // Response
    res->code = 230;
    strncpy(res->message, "User logged in", BUFFER_SIZE);

    return 0;
}

int ftp_user(response_t * res, const command_t * cmd, user_session_t * session) {
    FILE * fp;
    char buffer[BUFFER_SIZE];
    
    short int nComparedChars;
    short int isMatching = 1;

    // If user attempts to log as another user
    if (session->state == LOGGED_IN) {
        res->code = 230;
        strncpy(res->message, "User logged in", BUFFER_SIZE);

        return 0;
    }

    fp = fopen("passwd", "r");

        // If file did not open for any reason
        if (fp == NULL) {
            res->code = 504;
            strncpy(res->message, "Command not implemented for that parameter.", BUFFER_SIZE); 

            return -1;  
        }

        // Check the validity of the username
        while(isMatching != 0 && fgets(buffer, BUFFER_SIZE, fp) != NULL) {
            char * validName = strtok(buffer, ":");
            
            nComparedChars = (strlen(validName) < strlen(cmd->args))? strlen(validName) : strlen(cmd->args);
            isMatching = strncmp(validName, cmd->args, nComparedChars);
        }

    fclose(fp);

    if (isMatching == 0 && nComparedChars == strlen(cmd->args)) {
        session->state = NEEDS_PASSWORD;
        strncpy(session->username, res->message, BUFFER_SIZE);

        res->code = 331;
        strncpy(res->message, "User name okay, need password", BUFFER_SIZE);
    } else {
        session->state = LOGGED_OUT;
        memset(res->message, 0, BUFFER_SIZE);

        res->code = 530;
        strncpy(res->message, "User not logged in", BUFFER_SIZE);   
    }

    return 0;
}

int ftp_quit(response_t * res) {
    res->code = 221;
    strncpy(res->message, "Service closing control connection", BUFFER_SIZE);

    return 0;
}