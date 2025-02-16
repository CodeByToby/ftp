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

int ftp_list(response_t * res) {
    res->code = 502;
    strncpy(res->message, "Command not yet implemented", BUFFER_SIZE);

    return 0;
}

int ftp_noop(response_t * res) {
    int nSent;

    res->code = 200;

    return 0;
}