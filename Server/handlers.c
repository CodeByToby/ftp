#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h> // realpath
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

int ftp_rmd(response_t * res, const command_t * cmd, const user_session_t * session) {
    if (session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");
        return -1;  
    }

    response_set(res, 502, "Command not yet implemented");
    return 0;
}

int ftp_mkd(response_t * res, const command_t * cmd, const user_session_t * session) {
    if (session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");
        return -1;  
    }

    

    response_set(res, 502, "Command not yet implemented");
    return 0;
}

int ftp_pwd(response_t * res, const user_session_t * session) {
    if (session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");
        return -1;
    }

    response_set(res, 257, session->dir);
    return 0;
}

int ftp_cwd(response_t * res, const command_t * cmd, const user_session_t * session) {
    char oldPath[BUFFER_SIZE];
    char newPath[BUFFER_SIZE];
    char resolvedPath[BUFFER_SIZE];

    memset(oldPath, 0, sizeof(oldPath));
    memset(newPath, 0, sizeof(newPath));
    memset(resolvedPath, 0, sizeof(resolvedPath));

    if (session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");
        return -1;
    }

    if (cmd->args[0] == '\0') {
        response_set(res, 501, "Syntax error in parameters or arguments");
        return -1;
    }

    if (cmd->args[0] == '/') {
        // Parse as absolute path
        if (snprintf(newPath, sizeof(newPath), "%s/%s", session->root, cmd->args) > sizeof(newPath)) {
            response_set(res, 550, "Failed to change directory. Argument too long");
            return -1;
        }
    } else {
        // Parse as relative path
        getcwd(oldPath, sizeof(oldPath));

        if (snprintf(newPath, sizeof(newPath), "%s/%s", oldPath, cmd->args) > sizeof(newPath)) {
            response_set(res, 550, "Failed to change directory. Argument too long");
            return -1;
        }
    }

    // Resolve path, get rid of . and .. and additional slashes
    if (realpath(newPath, resolvedPath) == NULL) {
        response_set(res, 550, "Failed to change directory. Path is invalid or does not exist");
        return -1;
    }

    // Check if path is outside of root
    if (strncmp(session->root, resolvedPath, strlen(session->root)) != 0) {
        response_set(res, 550, "Failed to change directory. Directory out of scope");
        return -1;
    }

    // Change directory
    if (chdir(newPath) < 0) {
        tstamp(stderr);
        fprintf(stderr, " - [CLIENT_%d / ERRO] - ", getpid());
        perror("chdir()"); 

        response_set(res, 550, "Failed to change directory");
        return -1;
    }

    // Update directory in session
    const char * newDir = resolvedPath + strlen(session->root);

    if (newDir[0] != '\0')
        strncpy(session->dir, newDir, BUFFER_SIZE);
    else
        strncpy(session->dir, "/", 2);

    response_set(res, 250, "Requested action okay, completed");
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

    // Create a home directory for user
    char * root_path = strtok(NULL, "\n");

    if(mkdir(root_path, 0700) && errno != EEXIST) {
        tstamp(stderr);
        fprintf(stderr, " - [CLIENT_%d / ERRO] - ", getpid());
        perror("mkdir()"); 

        response_set(res, 550, "Requested action not taken. System issue");
        return -1;
    } 
    
    // Move the process to the root dir
    if(chdir(root_path)) {
        tstamp(stderr);
        fprintf(stderr, " - [CLIENT_%d / ERRO] - ", getpid());
        perror("chdir()"); 

        response_set(res, 550, "Requested action not taken. System issue");
        return -1;
    } 

    session->state = LOGGED_IN;
    getcwd(session->root, BUFFER_SIZE);
    strncpy(session->dir, "/", 2);

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