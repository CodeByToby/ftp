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

int ftp_list(response_t * res, user_session_t * session) {
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
    int nSent;

    res->code = 200;
    strncpy(res->message, "Command okay", BUFFER_SIZE);

    return 0;
}

int ftp_pass(response_t * res, user_session_t * session) {
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

int ftp_user(response_t * res, user_session_t * session) {
    //TODO: Handle login

    // Session
    session->state = NEEDS_PASSWORD;

    // Response
    res->code = 331;
    strncpy(res->message, "User name okay, need password", BUFFER_SIZE);

    return 0;
}
