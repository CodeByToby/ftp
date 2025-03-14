#include <stdio.h> // printf fopen fclose opendir
#include <stdlib.h> // realpath NULL
#include <unistd.h> // getpid getcwd chdir access rmdir
#include <string.h> // strncpy memset strlen strtok
#include <errno.h> // errno
#include <grp.h> // getgrgid
#include <pwd.h> // getpwuid
#include <time.h> // strftime
#include <dirent.h> // dirent
#include <sys/types.h>
#include <sys/stat.h> // mkdir stat
#include <sys/socket.h> // send

#include "../Common/packets.h"
#include "../Common/defines.h"
#include "../Common/string_helpers.h"
#include "sockets_and_inet.h"
#include "user_lock.h"
#include "commands.h"
#include "log.h"

// Working file / directory path
char fpath[BUFFER_SIZE];


void response_set(response_t * res, const int code, const char * message) {
    res->code = code;
    strncpy(res->message, message, BUFFER_SIZE-1);
    res->message[BUFFER_SIZE-1] = '\0';
}

static int __update_fpath(const command_t * cmd, response_t * res, const user_session_t * session, char * resolvedPath, size_t path_len) {
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

#define update_fpath() __update_fpath(cmd, res, session, fpath, sizeof(fpath))


static int stat_parse(const struct stat * fstat, const char * name, char * result) {
    char permissions[11];
    int links;
    struct passwd * owner;
    struct group * grp;
    off_t size;
    char mod_time[20];
    struct tm *time_info;

    // Get file type and permissions
    permissions[0] = (S_ISDIR(fstat->st_mode)) ? 'd' : '-';
    permissions[1] = (fstat->st_mode & S_IRUSR) ? 'r' : '-';
    permissions[2] = (fstat->st_mode & S_IWUSR) ? 'w' : '-';
    permissions[3] = (fstat->st_mode & S_IXUSR) ? 'x' : '-';
    permissions[4] = (fstat->st_mode & S_IRGRP) ? 'r' : '-';
    permissions[5] = (fstat->st_mode & S_IWGRP) ? 'w' : '-';
    permissions[6] = (fstat->st_mode & S_IXGRP) ? 'x' : '-';
    permissions[7] = (fstat->st_mode & S_IROTH) ? 'r' : '-';
    permissions[8] = (fstat->st_mode & S_IWOTH) ? 'w' : '-';
    permissions[9] = (fstat->st_mode & S_IXOTH) ? 'x' : '-';
    permissions[10] = '\0';

    // Get nr of links
    links = fstat->st_nlink;

    // Get the owner and group names
    owner = getpwuid(fstat->st_uid);
    grp = getgrgid(fstat->st_gid);

    // Get the file size
    size = fstat->st_size;

    // Get the modification time
    time_info = localtime(&fstat->st_mtime);
    strftime(mod_time, sizeof(mod_time), "%b %d %H:%M", time_info);

    // Print the formatted output
    sprintf(result, "%s %2d %-8s %-8s %5lld %s %s\n",
           permissions, links, owner ? owner->pw_name : "unknown", 
           grp ? grp->gr_name : "unknown", (long long)size, mod_time, name);

    return 0;
}
 
int ftp_list(response_t * res, const command_t * cmd, user_session_t * session, struct stat * fstat) {
    
    if(session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");   
        return -1;
    }

    if(session->conn_type == DATCONN_UNSET || session->sockfd_data <= 0) {
        response_set(res, 425, "Can't open data connection");
        return -1; 
    }

    if(update_fpath() < 0) {
        // response is set in function
        return -1;
    }

    if(stat(fpath, fstat) != 0) {
        log_erro("stat()", getpid());
        response_set(res, 550, "Requested action not taken. System issue");
        return -1;
    }

    response_set(res, 150, "File status okay; about to open data connection");

    return 0;
}

int ftp_list_data(response_t * res, const command_t * cmd, user_session_t * session, struct stat * fstat) {
    int retval = 0;
    int sockfd_data_accpt;

    // ACCEPT DATA CONNECTION

    if((sockfd_data_accpt = accept(session->sockfd_data, NULL, NULL)) <= 0) {
        log_erro("accept()", getpid());
        response_set(res, 425, "Can't open data connection");

        retval = -1;
        goto cleanup_list;
    }

    // LIST ALL FILES AND SUBDIRECTORIES

    char res_buffer[BUFFER_SIZE];

    if(S_ISDIR(fstat->st_mode)) {
        DIR * dir;
        struct dirent * entry;
        struct stat entry_stat;
        char entry_path[BUFFER_SIZE];

        if((dir = opendir(fpath)) == NULL) { 
            log_erro("opendir()", getpid());
            response_set(res, 550, "Requested action not taken. System issue");

            goto cleanup_list;
        }
            while ((entry = readdir(dir)) != NULL) {
                if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
                
                if(snprintf(entry_path, sizeof(entry_path), "%s/%s", fpath, entry->d_name) > sizeof(entry_path)) { 
                    response_set(res, 451, "Requested action aborted: local error in processing");
                    log_erro("snprintf()", getpid());

                    retval = -1;
                    goto cleanup_list;
                }

                if(stat(entry_path, &entry_stat) != 0) {
                    response_set(res, 451, "Requested action aborted: local error in processing");
                    log_erro("stat()", getpid());

                    retval = -1;
                    goto cleanup_list;
                }

                if(stat_parse(&entry_stat, entry->d_name, res_buffer) < 0) {
                    response_set(res, 451, "Requested action aborted: local error in processing");
                    log_erro("stat_parse()", getpid());

                    retval = -1;
                    goto cleanup_list;
                }

                int nSent = send(sockfd_data_accpt, res_buffer, sizeof(res_buffer), 0);

                if(nSent < 0 || nSent != sizeof(res_buffer)) {
                    response_set(res, 426, "Connection closed; transfer aborted");
                    log_erro("send()", getpid());

                    retval = -1;
                    goto cleanup_list;
                }
            }

        response_set(res, 226, "Closing data connection. Requested file action successful");
    }

    // ... OR GET INFO ON FILE

    else {
        if(stat_parse(fstat, cmd->args, res_buffer) < 0) {
                response_set(res, 451, "Requested action aborted: local error in processing");
                log_erro("stat_parse()", getpid());

                retval = -1;
                goto cleanup_list;
            }

            int nSent = send(sockfd_data_accpt, res_buffer, sizeof(res_buffer), 0);

            if(nSent < 0 || nSent != sizeof(res_buffer)) {
                response_set(res, 426, "Connection closed; transfer aborted");
                log_erro("send()", getpid());

                retval = -1;
                goto cleanup_list;
            }

        response_set(res, 226, "Closing data connection. Requested file action successful");
    }

    cleanup_list:
        close(sockfd_data_accpt);
        close(session->sockfd_data);
        session->sockfd_data = 0;
        session->conn_type = DATCONN_UNSET;

        return retval;
}

int ftp_retr(response_t * res, const command_t * cmd, user_session_t * session, FILE ** fptr) {
    if(session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");
        return -1;
    }

    if(session->conn_type == DATCONN_UNSET || session->sockfd_data <= 0) {
        response_set(res, 425, "Can't open data connection");
        return -1; 
    }

    if(cmd->args[0] == '\0') {
        response_set(res, 501, "Syntax error in parameters or arguments. No argument");
        return -1;
    }

    if(update_fpath() < 0) {
        // response is set in function
        return -1;
    }

    if((*fptr = fopen(fpath, "r")) == NULL) {
        response_set(res, 550, "Requested action not taken. File unavailable");
        return -1;   
    }

    response_set(res, 150, "File status okay; about to open data connection");

    return 0;
}

int ftp_retr_data(response_t * res, const command_t * cmd, user_session_t * session, FILE ** fptr) {
    int retval = 0;
    int sockfd_data_accpt;

    // ACCEPT DATA CONNECTION

    if((sockfd_data_accpt = accept(session->sockfd_data, NULL, NULL)) <= 0) {
        log_erro("accept()", getpid());
        response_set(res, 425, "Can't open data connection");

        retval = -1;
        goto cleanup_list;
    }

    // SEND CONTENTS OF FILE

    char res_buffer[BUFFER_SIZE];

    while(feof(*fptr) == 0) {
        memset(res_buffer, 0, sizeof(res_buffer));

        fread(res_buffer, sizeof(res_buffer), 1, *fptr);

        if(ferror(*fptr) != 0) {
            log_erro("fread()", getpid());
            response_set(res, 451, "Requested action aborted. Local error in processing");

            retval = -1;
            goto cleanup_list;
        }

        if(res_buffer[0] != '\0') {
            int nSent = send(sockfd_data_accpt, res_buffer, sizeof(res_buffer), 0);

            if(nSent < 0 || nSent != sizeof(res_buffer)) {
                response_set(res, 426, "Connection closed; transfer aborted");
                log_erro("send()", getpid());

                retval = -1;
                goto cleanup_list;
            }
        }
    }

    response_set(res, 226, "Closing data connection. Requested file action successful");

    cleanup_list:
        fclose(*fptr);
        close(sockfd_data_accpt);
        close(session->sockfd_data);
        session->sockfd_data = 0;
        session->conn_type = DATCONN_UNSET;

        return retval;
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

    if(access(cmd->args, F_OK) < 0) {
        response_set(res, 550, "Requested action not taken. Folder not found");
        return -1;
    }

    if(rmdir(cmd->args)) {
        if(errno == ENOTDIR)
            response_set(res, 550, "Requested action not taken. Not a directory");
        else if(errno == ENOTEMPTY)
            response_set(res, 550, "Requested action not taken. Not empty");
        else {
            log_erro("rmdir()", getpid());
            response_set(res, 550, "Requested action not taken. System issue");
        }
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

    if(access(cmd->args, F_OK) == 0) {
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
    if(session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");
        return -1;
    }

    if(cmd->args[0] == '\0') {
        response_set(res, 501, "Syntax error in parameters or arguments. No argument");
        return -1;
    }

    if(update_fpath() < 0) {
        // response is set in function
        return -1;
    }

    // Change directory
    if(chdir(fpath) < 0) {
        log_erro("chdir()", getpid());
        response_set(res, 550, "Failed to change directory");
        return -1;
    }

    // Update directory in session
    const char * newDir = fpath + strlen(session->root);

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
    if(lock_trywait(locks, session->username) < 0) {
        if(errno != EAGAIN)
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

    // Update session
    session->state = LOGGED_IN;
    session->sockfd_data = 0;
    session->conn_type = DATCONN_UNSET;
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

int ftp_quit(response_t * res) {
    response_set(res, 221, "Service closing control connection");
    return 0;
}

int ftp_help(response_t * res, const command_t * cmd) {
    if(cmd->args[0] != '\0') {
        // TODO: per-command help response?
        response_set(res, 504, "Command not implemented for that parameter");
        return -1;
    }

    response_set(res, 214, "Commands supported:\n\tNOOP\tQUIT\tHELP\n\tPASS\tUSER\n\tRETR\n\tPWD\tMKD\tRMD\tCWD\tCDUP\n\tPASV\n\tLIST\n");
    return 0;
}

int ftp_pasv(response_t * res, user_session_t * session) {
    int sockfd;

    ipv4_addr_t ipv4;
    port_t data_port;
    char data_port_str[10];

    if(session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");
        return -1;
    }

    // Reset data connection
    if(session->conn_type != DATCONN_UNSET || session->sockfd_data > 0) {
        close(session->sockfd_data);
    }

    ipv4_get(&ipv4);
    port_get(&data_port, data_port_str, 10);

    int i;
    const int MAX_TRIES = 10;

    for(i = 0; (sockfd = socket_create(data_port_str, -1)) < 0 && i < MAX_TRIES; ++i) {
        port_get(&data_port, data_port_str, 10);
        sleep(1);
    }

    if(i >= MAX_TRIES) {
        response_set(res, 425, "Can't open data connection");
        return -1;
    }

    session->sockfd_data = sockfd;
    session->conn_type = DATCONN_PASV;

    char msg[BUFFER_SIZE];
    snprintf(msg, sizeof(msg), "Entering passive mode. %d,%d,%d,%d,%d,%d", 
            ipv4.p1, ipv4.p2, ipv4.p3, ipv4.p4, data_port.p1, data_port.p2);

    response_set(res, 227, msg);

    return 0;
}

int ftp_port(response_t * res, const command_t * cmd, user_session_t * session) {
    if(session->state != LOGGED_IN) {
        response_set(res, 530, "User not logged in");
        return -1;
    }

    if(cmd->args[0] == '\0') {
        response_set(res, 501, "Syntax error in parameters or arguments. No argument");
        return -1;
    }

    response_set(res, 502, "Command not yet implemented");

    return 0;
}