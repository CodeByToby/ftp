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

int ftp_list(int sockfd_accpt, response_t * res) {
    int nSent;

    // Prepare response
    res->code = 502;
    strncpy(res->message, "Command not yet implemented", BUFFER_SIZE);

    // Send response
    nSent = send(sockfd_accpt, (void*) res, sizeof(response_t), 0);

    if (nSent < 0 || nSent != sizeof(response_t)) {
        fprintf(stderr, "[CLIENT_%d] ", getpid());
        perror("send()");

        close(sockfd_accpt);
        exit(EXIT_FAILURE);
    }  

    return 0;
}

int ftp_noop(int sockfd_accpt, response_t * res) {
    int nSent;

    // Prepare response
    res->code = 200;

    // Send response
    nSent = send(sockfd_accpt, (void*) res, sizeof(response_t), 0);

    if (nSent < 0 || nSent != sizeof(&res)) {
        close(sockfd_accpt);

        printf("[CLIENT_%d] ", getpid());
        perror("send()");
        exit(EXIT_FAILURE);
    }  

    return 0;
}