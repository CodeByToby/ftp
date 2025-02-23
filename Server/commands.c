#include <stdio.h> // printf fopen fclose
#include <stdlib.h> // realpath NULL
#include <unistd.h> // getpid getcwd chdir access rmdir
#include <string.h> // strncpy memset strlen strtok
#include <errno.h> // errno
#include <sys/types.h>
#include <sys/stat.h> // mkdir

#include "../Common/packets.h"
#include "../Common/defines.h"
#include "../Common/string_helpers.h"
#include "user_lock.h"
#include "commands.h"
#include "log.h"

void response_set(response_t * res, const int code, const char * message) {
    res->code = code;
    strncpy(res->message, message, BUFFER_SIZE-1);
    res->message[BUFFER_SIZE-1] = '\0';
}


int ftp_list(response_t * res, const command_t * cmd, const user_session_t * session) {
    if(session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");
        return -1;  
    }

    response_set(res, 502, "Command not yet implemented");
    return 0;
}

int ftp_rmd(response_t * res, const command_t * cmd, const user_session_t * session) {
    const char allowedCharacters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-";
    
    if(session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");
        return -1;  
    }

    if(cmd->args[0] == '\0') {
        response_set(res, 501, "Syntax error in parameters or arguments. No argument");
        return -1;
    }

    if(strspn(cmd->args, allowedCharacters) != strlen(cmd->args)) {
        response_set(res, 501, "Syntax error in parameters or arguments. Folder name is not correct");
        return -1;
    }

    if (access(cmd->args, F_OK) < 0) {
        response_set(res, 550, "Requested action not taken. Folder not found");
        return -1;
    }

    if(rmdir(cmd->args)) {
        log_erro("rmdir()", getpid());
        response_set(res, 550, "Requested action not taken. System issue");
        return -1;
    }

    response_set(res, 250, "Requested action okay, completed");
    return 0;
}

int ftp_mkd(response_t * res, const command_t * cmd, const user_session_t * session) {
    const char allowedCharacters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-";

    if(session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");
        return -1;
    }

    if(cmd->args[0] == '\0') {
        response_set(res, 501, "Syntax error in parameters or arguments. No argument");
        return -1;
    }

    if(strspn(cmd->args, allowedCharacters) != strlen(cmd->args)) {
        response_set(res, 501, "Syntax error in parameters or arguments. Folder name is not correct");
        return -1;
    }

    if (access(cmd->args, F_OK) == 0) {
        response_set(res, 550, "Requested action not taken. Folder already exists");
        return -1;
    }

    if(mkdir(cmd->args, 0700)) {
        log_erro("mkdir()", getpid());
        response_set(res, 550, "Requested action not taken. System issue");
        return -1;
    }

    response_set(res, 250, "Requested action okay, completed");
    return 0;
}

int ftp_pwd(response_t * res, const user_session_t * session) {
    if(session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");
        return -1;
    }

    response_set(res, 257, session->dir);
    return 0;
}

int ftp_cwd(response_t * res, const command_t * cmd, user_session_t * session) {
    char oldPath[BUFFER_SIZE];
    char newPath[BUFFER_SIZE];
    char resolvedPath[BUFFER_SIZE];

    memset(oldPath, 0, sizeof(oldPath));
    memset(newPath, 0, sizeof(newPath));
    memset(resolvedPath, 0, sizeof(resolvedPath));

    if(session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");
        return -1;
    }

    if(cmd->args[0] == '\0') {
        response_set(res, 501, "Syntax error in parameters or arguments. No argument");
        return -1;
    }

    if(cmd->args[0] == '/') {
        // Parse as absolute path
        if(snprintf(newPath, sizeof(newPath), "%s/%s", session->root, cmd->args) > sizeof(newPath)) {
            response_set(res, 550, "Failed to change directory. Argument too long");
            return -1;
        }
    } else {
        // Parse as relative path
        getcwd(oldPath, sizeof(oldPath));

        if(snprintf(newPath, sizeof(newPath), "%s/%s", oldPath, cmd->args) > sizeof(newPath)) {
            response_set(res, 550, "Failed to change directory. Argument too long");
            return -1;
        }
    }

    // Resolve path, get rid of . and .. and additional slashes
    if(realpath(newPath, resolvedPath) == NULL) {
        response_set(res, 550, "Failed to change directory. Path is invalid or does not exist");
        return -1;
    }

    // Check if path is outside of root
    if(strncmp(session->root, resolvedPath, strlen(session->root)) != 0) {
        response_set(res, 550, "Failed to change directory. Directory out of scope");
        return -1;
    }

    // Change directory
    if(chdir(newPath) < 0) {
        log_erro("chdir()", getpid());
        response_set(res, 550, "Failed to change directory");
        return -1;
    }

    // Update directory in session
    const char * newDir = resolvedPath + strlen(session->root);

    if(newDir[0] != '\0') {
        strncpy(session->dir, newDir, BUFFER_SIZE-1);
        session->dir[BUFFER_SIZE-1] = '\0';
    } else {
        strncpy(session->dir, "/", 2);
    }

    response_set(res, 250, "Requested action okay, completed");
    return 0;
}

int ftp_noop(response_t * res) {
    response_set(res, 200, "Command okay");
    return 0;
}

int ftp_pass(response_t * res, const command_t * cmd, user_session_t * session, const user_lock_array_t * locks) {
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
        log_erro("fopen()", getpid());
        response_set(res, 550, "Requested action not taken. System issue");
        return -1;  
    }

        // Check credentials for the user
        while (isValid == FALSE && fgets(buffer, BUFFER_SIZE, fp) != NULL) {
            char * validName = strtok(buffer, ":");
            isValid = strncmp_size(validName, session->username);

            if(isValid == FALSE)
                continue;

            char * validPass = strtok(NULL, ":");
            isValid = strncmp_size(validPass, cmd->args);
        }

    fclose(fp);

    if(isValid == FALSE) {
        session->state = NEEDS_PASSWORD;

        response_set(res, 530, "User not logged in");
        return -1;
    }

    // Attempt to lock user
    if (lock_trywait(locks, session->username) < 0) {
        if (errno != EAGAIN)
            log_erro("lock_trywait()", getpid());
        response_set(res, 550, "Requested action not taken. User is already logged in");
        return -1;       
    }

    // Create a home directory for user
    char * root_path = strtok(NULL, "\n");

    if(mkdir(root_path, 0700) && errno != EEXIST) {
        log_erro("mkdir()", getpid());
        response_set(res, 550, "Requested action not taken. System issue");
        return -1;
    } 
    
    // Move the process to the root dir
    if(chdir(root_path)) {
        log_erro("chdir()", getpid());
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
        log_erro("fopen()", getpid());
        response_set(res, 550, "Requested action not taken. System issue");
        return -1;  
    }

        // Check the validity of the username
        while (isValid == FALSE && fgets(buffer, BUFFER_SIZE, fp) != NULL) {
            char * validName = strtok(buffer, ":");
            isValid = strncmp_size(validName, cmd->args);
        }

    fclose(fp);

    if(isValid == FALSE) {
        session->state = LOGGED_OUT;
        memset(res->message, 0, BUFFER_SIZE);

        response_set(res, 550, "User not logged in");
        return -1;
    }

    session->state = NEEDS_PASSWORD;
    strncpy(session->username, cmd->args, BUFFER_SIZE-1);
    session->username[BUFFER_SIZE-1] = '\0';

    response_set(res, 331, "User name okay, need password");
    return 0;
}

int ftp_quit(response_t * res, const user_session_t * session, const user_lock_array_t * locks) {
    // Unlock user
    lock_post(locks, session->username);
    lock_close(locks, session->username);

    response_set(res, 221, "Service closing control connection");
    return 0;
}

int ftp_help(response_t * res, const command_t * cmd) {
    if(cmd->args[0] != '\0') {
        // TODO: per-command help response?
        response_set(res, 504, "Command not implemented for that parameter");
        return -1;
    }

    response_set(res, 214, "Commands supported:\n\tNOOP\tQUIT\tHELP\n\tPASS\tUSER\n\tPWD\tMKD\tRMD\tCWD\tCDUP\n");
    return 0;
}