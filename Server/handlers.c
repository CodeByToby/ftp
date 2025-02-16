#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../Common/packets.h"
#include "../Common/defines.h"
#include "handlers.h"

#define CREDENTIALS_FILE "passwd"

static int validate(const char * validStr, const char * str) {
    int validStrLen = strlen(validStr);

    return (strncmp(validStr, str, validStrLen) == 0 && 
            validStrLen == strlen(str))? TRUE : FALSE;
}

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
    FILE * fp;
    char buffer[BUFFER_SIZE];
    
    short int isValid = FALSE;

    // If user attempts to log as another user
    if (session->state == LOGGED_IN) {
        res->code = 230;
        strncpy(res->message, "User logged in", BUFFER_SIZE);

        return -1;
    }

    // If the user has not provided a login yet
    if (session->state == LOGGED_OUT) {
        res->code = 332;
        strncpy(res->message, "Need account for login.", BUFFER_SIZE); 

        return -1;
    }

    fp = fopen(CREDENTIALS_FILE, "r");

        // If file did not open for any reason
        if (fp == NULL) {
            res->code = 504;
            strncpy(res->message, "Command not implemented for that parameter", BUFFER_SIZE); 

            return -1;  
        }

        // Check credentials for the user
        while (isValid == FALSE && fgets(buffer, BUFFER_SIZE, fp) != NULL) {
            char * validName = strtok(buffer, ":");
            isValid = validate(validName, session->username);

            if (isValid == FALSE) {
                continue;
            }

            char * validPass = strtok(NULL, ":");
            isValid = validate(validPass, cmd->args);
        }

    fclose(fp);

    if (isValid == TRUE) {
        session->state = LOGGED_IN;
        strncpy(session->path, strtok(NULL, ""), BUFFER_SIZE); 

        res->code = 230;
        strncpy(res->message, "User logged in", BUFFER_SIZE);

        return 0;
    } else {
        session->state = NEEDS_PASSWORD;

        res->code = 530;
        strncpy(res->message, "User not logged in", BUFFER_SIZE);

        return -1;
    }

}

int ftp_user(response_t * res, const command_t * cmd, user_session_t * session) {
    FILE * fp;
    char buffer[BUFFER_SIZE];
    
    short int isValid = FALSE;

    // If user attempts to log as another user
    if (session->state == LOGGED_IN) {
        res->code = 230;
        strncpy(res->message, "User logged in", BUFFER_SIZE);

        return -1;
    }

    fp = fopen(CREDENTIALS_FILE, "r");

        // If file did not open for any reason
        if (fp == NULL) {
            res->code = 504;
            strncpy(res->message, "Command not implemented for that parameter", BUFFER_SIZE); 

            return -1;  
        }

        // Check the validity of the username
        while (isValid == FALSE && fgets(buffer, BUFFER_SIZE, fp) != NULL) {
            char * validName = strtok(buffer, ":");
            isValid = validate(validName, cmd->args);
        }

    fclose(fp);

    if (isValid == TRUE) {
        session->state = NEEDS_PASSWORD;
        strncpy(session->username, cmd->args, BUFFER_SIZE);

        res->code = 331;
        strncpy(res->message, "User name okay, need password", BUFFER_SIZE);

        return 0;
    } else {
        session->state = LOGGED_OUT;
        memset(res->message, 0, BUFFER_SIZE);

        res->code = 530;
        strncpy(res->message, "User not logged in", BUFFER_SIZE);   
    
        return -1;
    } 
}

int ftp_quit(response_t * res) {
    res->code = 221;
    strncpy(res->message, "Service closing control connection", BUFFER_SIZE);

    return 0;
}