#include <stdio.h> // printf fopen fclose opendir
#include <stdlib.h> // realpath NULL
#include <unistd.h> // getpid getcwd chdir access rmdir
#include <string.h> // strncpy memset strlen strtok
#include <errno.h> // errno
#include <dirent.h> // dirent
#include <sys/types.h>
#include <sys/stat.h> // mkdir stat

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

static int resolve_path(const command_t * cmd, response_t * res, const user_session_t * session, char * resolvedPath, size_t path_len) {
    char newPath[path_len];
    char oldPath[path_len];

    memset(newPath, 0, path_len);
    memset(oldPath, 0, path_len);

    if(cmd->args[0] == '/') {
        // Parse as absolute path
        if(snprintf(newPath, path_len, "%s/%s", session->root, cmd->args) > path_len) {
            response_set(res, 550, "Requested action not taken. Argument too long");
            return -1;
        }
    } else {
        // Parse as relative path
        getcwd(oldPath, sizeof(oldPath));

        if(snprintf(newPath, path_len, "%s/%s", oldPath, cmd->args) > path_len) {
            response_set(res, 550, "Requested action not taken. Argument too long");
            return -1;
        }
    }

    // Resolve path, get rid of . and .. and additional slashes
    if(realpath(newPath, resolvedPath) == NULL) {
        response_set(res, 550, "Requested action not taken. Path is invalid or does not exist");
        return -1;
    }

    // Check if path is outside of root
    if(strncmp(session->root, resolvedPath, strlen(session->root)) != 0) {
        response_set(res, 550, "Requested action not taken. Path out of scope");
        return -1;
    }

    return 0;
}

#define get_resolved_path(path) resolve_path(cmd, res, session, path, sizeof(path))


int ftp_list(response_t * res, const command_t * cmd, const user_session_t * session) {
    char path[BUFFER_SIZE];
    struct stat pathStat;

    memset(path, 0, sizeof(path));
    
    if(session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");
        return -1;  
    }

    if (get_resolved_path(path) < 0) {
        // response is set in function
        return -1;
    }

    if(stat(path, &pathStat) != 0) {
        log_erro("stat()", getpid());
        response_set(res, 550, "Requested action not taken. System issue");
        return -1;
    }

    if(S_ISDIR(pathStat.st_mode)) {
        // LIST ALL FILES AND SUBDIRECTORIES

        char entryPath[BUFFER_SIZE];

        DIR * dir;
        struct dirent * entry;
        struct stat entryStat;

        if ((dir = opendir(path)) == NULL) { 
            log_erro("opendir()", getpid());
            response_set(res, 550, "Requested action not taken. System issue");
            return -1;
        }

            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
                
                if(snprintf(entryPath, sizeof(entryPath), "%s/%s", path, entry->d_name) > sizeof(entryPath)) { 
                    response_set(res, 550, "Requested action not taken. Filename too long to output");
                    return -1;
                }

                if(stat(entryPath, &entryStat) != 0) {
                    log_erro("stat()", getpid());
                    response_set(res, 550, "Requested action not taken. System issue");
                    return -1;
                }

                if (S_ISDIR(entryStat.st_mode)) {
                    printf("%s/\n", entry->d_name);
                }
                else {
                    printf("%s\n", entry->d_name);
                }
            }

        closedir(dir);

        response_set(res, 200, path);
        return 0;
    }

    else {
        // GET INFO ON FILE

        response_set(res, 200, path);
        return 0;
    }
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
    char path[BUFFER_SIZE];

    memset(path, 0, sizeof(path));

    if(session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");
        return -1;
    }

    if(cmd->args[0] == '\0') {
        response_set(res, 501, "Syntax error in parameters or arguments. No argument");
        return -1;
    }

    if (get_resolved_path(path) < 0) {
        // response is set in function
        return -1;
    }

    // Change directory
    if(chdir(path) < 0) {
        log_erro("chdir()", getpid());
        response_set(res, 550, "Failed to change directory");
        return -1;
    }

    // Update directory in session
    const char * newDir = path + strlen(session->root);

    if(newDir[0] != '\0') {
        strncpy(session->dir, newDir, BUFFER_SIZE-1);
        session->dir[BUFFER_SIZE-1] = '\0';
    } else {
        strncpy(session->dir, "/", 2); // realpath cuts off last /, so we add it back in in case we're in root
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