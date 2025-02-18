#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // mkdir
#include <errno.h> // errno

#include "../Common/packets.h"
#include "../Common/defines.h"
#include "handlers.h"
#include "tstamp.h"

#define CREDENTIALS_FILE "passwd"

static int validate(const char * validStr, const char * str) {
    int validStrLen = strlen(validStr);

    return (strncmp(validStr, str, validStrLen) == 0 && 
            validStrLen == strlen(str))? TRUE : FALSE;
}

static void response_set(response_t * res, const int code, const char * message) {
    res->code = code;
    strncpy(res->message, message, BUFFER_SIZE);
}

int ftp_list(response_t * res, const command_t * cmd, const user_session_t * session) {
    if (session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");
        return -1;  
    }

    response_set(res, 502, "Command not yet implemented");
    return 0;
}

int ftp_noop(response_t * res) {
    response_set(res, 200, "Command okay");
    return 0;
}

int ftp_pass(response_t * res, const command_t * cmd, user_session_t * session) {
    FILE * fp;
    char buffer[BUFFER_SIZE];
    
    short int isValid = FALSE;

    if(session->state != NEEDS_PASSWORD) {
        response_set(res, 530, 
            (session->state == LOGGED_IN)? 
                "Session already active" : 
                "User not logged in"
        );
        return -1;
    }

    if((fp = fopen(CREDENTIALS_FILE, "r")) == NULL) {
        tstamp(stderr);
        fprintf(stderr, " - [CLIENT_%d / ERRO] - ", getpid());
        perror("fopen()"); 

        response_set(res, 550, "Requested action not taken. System issue");
        return -1;  
    }

        // Check credentials for the user
        while (isValid == FALSE && fgets(buffer, BUFFER_SIZE, fp) != NULL) {
            char * validName = strtok(buffer, ":");
            isValid = validate(validName, session->username);

            if (isValid == FALSE)
                continue;

            char * validPass = strtok(NULL, ":");
            isValid = validate(validPass, cmd->args);
        }

    fclose(fp);

    if(isValid == FALSE) {
        session->state = NEEDS_PASSWORD;

        response_set(res, 530, "User not logged in");
        return -1;
    }

    session->state = LOGGED_IN;
    strncpy(session->path, strtok(NULL, "\n"), BUFFER_SIZE); 

    // Create a home directory for user
    if(mkdir(session->path, 0700) && errno != EEXIST) {
        tstamp(stderr);
        fprintf(stderr, " - [CLIENT_%d / ERRO] - ", getpid());
        perror("mkdir()"); 

        response_set(res, 550, "Requested action not taken. System issue");
        return -1;
    }

    response_set(res, 230, "User logged in");
    return 0;
}

int ftp_user(response_t * res, const command_t * cmd, user_session_t * session) {
    FILE * fp;
    char buffer[BUFFER_SIZE];
    
    short int isValid = FALSE;

    if(session->state == LOGGED_IN) {
        response_set(res, 530, "Session already active");
        return -1;
    }

    if((fp = fopen(CREDENTIALS_FILE, "r")) == NULL) {
        tstamp(stderr);
        fprintf(stderr, " - [CLIENT_%d / ERRO] - ", getpid());
        perror("fopen()"); 

        response_set(res, 550, "Requested action not taken. System issue");
        return -1;  
    }

        // Check the validity of the username
        while (isValid == FALSE && fgets(buffer, BUFFER_SIZE, fp) != NULL) {
            char * validName = strtok(buffer, ":");
            isValid = validate(validName, cmd->args);
        }

    fclose(fp);

    if(isValid == FALSE) {
        session->state = LOGGED_OUT;
        memset(res->message, 0, BUFFER_SIZE);

        response_set(res, 550, "User not logged in");
        return -1;
    }

    session->state = NEEDS_PASSWORD;
    strncpy(session->username, cmd->args, BUFFER_SIZE);

    response_set(res, 331, "User name okay, need password");
    return 0;
}

int ftp_quit(response_t * res) {
    response_set(res, 221, "Service closing control connection");
    return 0;
}

int ftp_help(response_t * res, const command_t * cmd) {
    if (cmd->args[0] != '\0') {
        // TODO: per-command help response?
        response_set(res, 504, "Command not implemented for that parameter");
        return -1;
    }

    response_set(res, 214, "Commands supported:\n\tNOOP\tQUIT\tHELP\n\tPASS\tUSER\n");
    return 0;
}