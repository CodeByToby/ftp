#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h> // mkdir
#include <errno.h> // errno

#include "../Common/packets.h"
#include "../Common/defines.h"
#include "tstamp.h"
#include "handlers.h"

static int response_send(int sockfd, response_t * res);

int main(int argc, char** argv) 
{
    int nRead;
    int retval;
    int sockfd,
        sockfd_accpt;
    const struct sockaddr_un addr = { AF_UNIX, "/tmp/ftp.sock" };

    // CREATE AN ENDPOINT

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) { 
        perror("socket()"); 
        exit(EXIT_FAILURE); 
    }

    // DEFINE TIMEOUT FOR MAIN SOCKET

    struct timeval tv;
    tv.tv_sec = 360;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0) {
        perror("setsockopt()"); 
        exit(EXIT_FAILURE);
    }

    // BIND THE SOCKET TO ADDRESS

    if (bind(sockfd, (struct sockaddr*) &addr, sizeof(addr)) < 0) { 
        perror("bind()"); 
        exit(EXIT_FAILURE);
    }

    // PREPARE TO ACCEPT CONNECTIONS

    if (listen(sockfd, 5) < 0) { 
        perror("listen()"); 
        exit(EXIT_FAILURE); 
    }

    // MAKE A HOME DIRECTORY FOR CLIENTS

    if(mkdir("home", 0700) && errno != EEXIST) {
        perror("mkdir()");
        exit(EXIT_FAILURE);
    }

    tstamp(stdout);
    printf(" - [SERVER / INFO] - Online\n");

    // SERVER LOOP

    command_t cmd;
    response_t res;

    while(TRUE) {
        memset(&cmd, 0, sizeof(cmd));
        memset(&res, 0, sizeof(res));

        // ACCEPT SOCKET

        if ((sockfd_accpt = accept(sockfd, NULL, NULL)) < 0) {
            tstamp(stderr);
            fprintf(stderr, " - [SERVER / ERRO] - ");
            perror("accept()"); 

            exit(EXIT_FAILURE); 
        }

        // FORK IN THE ROAD

        retval = fork();

        if (retval < 0) {
            fprintf(stderr, "<ERR> ");
            perror("fork()"); 

            close(sockfd_accpt);
        } else if (retval > 0) {
            continue;
        }

        // DEFINE TIMEOUT FOR ACCEPT SOCKET

        struct timeval tv;
        tv.tv_sec = 30;
        tv.tv_usec = 0;
        if (setsockopt(sockfd_accpt, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0) {
            tstamp(stderr);
            fprintf(stderr, " - [CLIENT_%d / ERRO] - ", getpid());
            perror("setsockopt()"); 

            exit(EXIT_FAILURE); 
        }

        // CLIENT-SPECIFIC SESSION

        user_session_t session;

        memset(&session, 0, sizeof(session));
        session.state = LOGGED_OUT;

        while (TRUE) {
            nRead = recv(sockfd_accpt, (command_t*) &cmd, sizeof(command_t), 0);

            if (nRead < 0) {
                tstamp(stderr);
                fprintf(stderr, " - [CLIENT_%d / ERRO] - ", getpid());
                perror("recv()"); 

                // TODO: Use existing function response_set? Put same logic in SIGINT handler?
                res.code = 421;
                strncpy(res.message, "Service not available, closing control connection", BUFFER_SIZE);
                send(sockfd_accpt, (void*) &res, sizeof(response_t), 0);

                close(sockfd_accpt);
                exit(EXIT_FAILURE);
            }

            switch (cmd.type) {
            case USER:
                tstamp(stdout);
                printf(" - [CLIENT_%d / COMM] - USER command received: USER %s\n", getpid(), cmd.args);

                ftp_user(&res, &cmd, &session);

                tstamp(stdout);
                printf(" - [CLIENT_%d / RESP] - %d %s\n", getpid(), res.code, res.message);
                
                response_send(sockfd_accpt, &res);

                break;
                
            case PASS:
                tstamp(stdout);
                printf(" - [CLIENT_%d / COMM] - PASS command received\n", getpid());
                
                ftp_pass(&res, &cmd, &session);

                tstamp(stdout);
                printf(" - [CLIENT_%d / RESP] - %d %s\n", getpid(), res.code, res.message);

                response_send(sockfd_accpt, &res);
                break;

            case LIST:
                tstamp(stdout);
                printf(" - [CLIENT_%d / COMM] - PASS command received: LIST %s\n", getpid(), cmd.args);

                ftp_list(&res, &cmd, &session);

                tstamp(stdout);
                printf(" - [CLIENT_%d / RESP] - %d %s\n", getpid(), res.code, res.message);

                response_send(sockfd_accpt, &res);
                break;

            case RMD:
                tstamp(stdout);
                printf(" - [CLIENT_%d / COMM] - RMD command received: RMD %s\n", getpid(), cmd.args);

                ftp_rmd(&res, &cmd, &session);

                tstamp(stdout);
                printf(" - [CLIENT_%d / RESP] - %d %s\n", getpid(), res.code, res.message);

                response_send(sockfd_accpt, &res);
                break;

            case MKD:
                tstamp(stdout);
                printf(" - [CLIENT_%d / COMM] - MKD command received: MKD %s\n", getpid(), cmd.args);

                ftp_mkd(&res, &cmd, &session);

                tstamp(stdout);
                printf(" - [CLIENT_%d / RESP] - %d %s\n", getpid(), res.code, res.message);

                response_send(sockfd_accpt, &res);
                break;

            case PWD:
                tstamp(stdout);
                printf(" - [CLIENT_%d / COMM] - PWD command received: PWD\n", getpid());

                ftp_pwd(&res, &session);

                tstamp(stdout);
                printf(" - [CLIENT_%d / RESP] - %d %s\n", getpid(), res.code, res.message);

                response_send(sockfd_accpt, &res);
                break;

            case CWD:
                tstamp(stdout);
                printf(" - [CLIENT_%d / COMM] - CWD command received: CWD %s\n", getpid(), cmd.args);

                ftp_cwd(&res, &cmd, &session);

                tstamp(stdout);
                printf(" - [CLIENT_%d / RESP] - %d %s\n", getpid(), res.code, res.message);

                response_send(sockfd_accpt, &res);
                break;

            case CDUP:
                tstamp(stdout);
                printf(" - [CLIENT_%d / COMM] - CDUP command received: CDUP\n", getpid());

                strncpy(cmd.args, "..", 3);
                ftp_cwd(&res, &cmd, &session);

                tstamp(stdout);
                printf(" - [CLIENT_%d / RESP] - %d %s\n", getpid(), res.code, res.message);

                response_send(sockfd_accpt, &res);
                break;

            case NOOP:
                tstamp(stdout);
                printf(" - [CLIENT_%d / COMM] - NOOP command received: NOOP\n", getpid());

                ftp_noop(&res);

                tstamp(stdout);
                printf(" - [CLIENT_%d / RESP] - %d %s\n", getpid(), res.code, res.message);

                response_send(sockfd_accpt, &res);
                break;

            case QUIT:
                tstamp(stdout);
                printf(" - [CLIENT_%d / COMM] - QUIT command received: QUIT\n", getpid());

                ftp_quit(&res);

                tstamp(stdout);
                printf(" - [CLIENT_%d / RESP] - %d %s\n", getpid(), res.code, res.message);

                response_send(sockfd_accpt, &res);
                
                close(sockfd_accpt);
                exit(EXIT_SUCCESS);

            case HELP:
            default:
                tstamp(stdout);
                printf(" - [CLIENT_%d / COMM] - HELP command received: HELP %s\n", getpid(), cmd.args);
                
                ftp_help(&res, &cmd);
                
                tstamp(stdout);
                printf(" - [CLIENT_%d / RESP] - %d\n", getpid(), res.code);

                response_send(sockfd_accpt, &res);
                break;
            }
        }
    }
}

static int response_send(int sockfd, response_t * res)
{    
    int nSent = send(sockfd, (void*) res, sizeof(response_t), 0);

    if (nSent < 0 || nSent != sizeof(response_t)) {
        tstamp(stderr);
        fprintf(stderr, " - [CLIENT_%d / ERRO] - ", getpid());
        perror("send()");

        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return 0;
}