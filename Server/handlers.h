#ifndef HANDLERS_H
#define HANDLERS_H

#include "../Common/packets.h"
#include "../Common/defines.h"

typedef enum {
    LOGGED_IN,
    LOGGED_OUT,
    NEEDS_PASSWORD
} user_state;

typedef struct user_session {
    user_state state;
    char username[BUFFER_SIZE];
    char path[BUFFER_SIZE];
} user_session_t;

int ftp_list(response_t * res, const command_t * cmd, const user_session_t * session);
int ftp_noop(response_t * res);
int ftp_quit(response_t * res);
int ftp_pass(response_t * res, const command_t * cmd, user_session_t * session);
int ftp_user(response_t * res, const command_t * cmd, user_session_t * session);

#endif