#ifndef COMMANDS_H
#define COMMANDS_H

#include "../Common/packets.h"
#include "../Common/defines.h"
#include "user_lock.h"

#define CREDENTIALS_FILE "passwd"

typedef enum {
    LOGGED_IN,
    LOGGED_OUT,
    NEEDS_PASSWORD
} user_state_t;

typedef enum {
    DATCONN_UNSET,
    DATCONN_PASV,
    DATCONN_ACTV
} data_conn_type_t;

typedef struct user_session {
    user_state_t state;

    char username[BUFFER_SIZE];
    char root[BUFFER_SIZE];
    char dir[BUFFER_SIZE];

    data_conn_type_t conn_type;
    int sockfd_data;
} user_session_t;

void response_set(response_t * res, const int code, const char * message);

int ftp_list(response_t * res, const command_t * cmd, user_session_t * session, struct stat * fdstat);
int ftp_list_data(response_t * res, const command_t * cmd, user_session_t * session, struct stat * fdstat);
int ftp_retr(response_t * res, const command_t * cmd, user_session_t * session, FILE ** fptr);
int ftp_retr_data(response_t * res, const command_t * cmd, user_session_t * session, FILE ** fptr);
int ftp_stor(response_t * res, command_t * cmd, user_session_t * session, FILE ** fptr);
int ftp_stor_data(response_t * res, const command_t * cmd, user_session_t * session, FILE ** fptr);
int ftp_rmd(response_t * res, const command_t * cmd, const user_session_t * session);
int ftp_mkd(response_t * res, const command_t * cmd, const user_session_t * session);
int ftp_pwd(response_t * res, const user_session_t * session);
int ftp_cwd(response_t * res, const command_t * cmd, user_session_t * session);
int ftp_noop(response_t * res);
int ftp_quit(response_t * res);
int ftp_pass(response_t * res, const command_t * cmd, user_session_t * session, const user_lock_array_t * locks);
int ftp_user(response_t * res, const command_t * cmd, user_session_t * session);
int ftp_help(response_t * res, const command_t * cmd);
int ftp_pasv(response_t * res, user_session_t * session);
int ftp_port(response_t * res, const command_t * cmd, user_session_t * session);

#endif