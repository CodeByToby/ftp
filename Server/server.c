#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include <wait.h>
#include <string.h>

#include "../Common/packets.h"
#include "../Common/defines.h"
#include "handlers.h"

int response_send (int sockfd, response_t * res);

int main (int argc, char** argv) 
{
    int nSent, nRead;
    int retval;
    int sockfd,
        sockfd_accpt;
    const struct sockaddr_un addr = { AF_UNIX, "/tmp/ftp.sock" };

    // CREATE AN ENDPOINT

    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) { 
        perror("socket()"); 
        exit(EXIT_FAILURE); 
    }

    // DEFINE TIMEOUT FOR SOCKET

    struct timeval tv;
    tv.tv_sec = 60;
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

    printf("[SERVER] online.\n");

    // SERVER LOOP

    command_t cmd;
    response_t res;

    while(TRUE) {
        memset(&cmd, 0, sizeof(cmd));
        memset(&res, 0, sizeof(res));

        if ((sockfd_accpt = accept(sockfd, NULL, NULL)) < 0) { 
            fprintf(stderr, "<ERR> ");
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

        // CLIENT-SPECIFIC SESSION

        user_session_t session;

        memset(&session, 0, sizeof(session));
        session.state = LOGGED_OUT;

        while (TRUE) {
            nRead = recv(sockfd_accpt, (command_t*) &cmd, sizeof(command_t), 0);

            if (nRead < 0) {
                fprintf(stderr, "<ERR> PID_%d: ", getpid());
                perror("recv()");

                close(sockfd_accpt);
                exit(EXIT_FAILURE);
            }

            switch (cmd.type) {
            case USER:
                printf("[CLIENT_%d] USER\n", getpid());

                ftp_user(&res, &session);
                response_send(sockfd_accpt, &res);
                break;
            case PASS:
                printf("[CLIENT_%d] PASS\n", getpid());

                ftp_pass(&res, &session);
                response_send(sockfd_accpt, &res);
                break;

            case LIST:
                printf("[CLIENT_%d] LIST\n", getpid());

                ftp_list(&res, &session);
                response_send(sockfd_accpt, &res);
                break;

            case RETR:
                printf("[CLIENT_%d] RETR\n", getpid());

                //ftp_retr(&res);
                //response_send(sockfd_accpt, &res);
                break;

            case STOR:
                printf("[CLIENT_%d] STOR\n", getpid());

                //ftp_stor(&res);
                //response_send(sockfd_accpt, &res);

                break;

            case STOU:
                printf("[CLIENT_%d] STOU\n", getpid());

                //ftp_stou(&res);
                //response_send(sockfd_accpt, &res);
                break;

            case APPE:
                printf("[CLIENT_%d] APPE\n", getpid());

                //ftp_appe(&res);
                //response_send(sockfd_accpt, &res);
                break;

            case DELE:
                printf("[CLIENT_%d] DELE\n", getpid());

                //ftp_dele(&res);
                //response_send(sockfd_accpt, &res);
                break;

            case RMD:
                printf("[CLIENT_%d] RMD\n", getpid());

                //ftp_rmd(&res);
                //response_send(sockfd_accpt, &res);
                break;

            case MKD:
                printf("[CLIENT_%d] MKD\n", getpid());

                //ftp_mkd(&res);
                //response_send(sockfd_accpt, &res);
                break;

            case PWD:
                printf("[CLIENT_%d] PWD\n", getpid());

                //ftp_pwd(&res);
                //response_send(sockfd_accpt, &res);
                break;

            case CWD:
                printf("[CLIENT_%d] CWD\n", getpid());

                //ftp_cwd(&res);
                //response_send(sockfd_accpt, &res);
                break;

            case CDUP:
                printf("[CLIENT_%d] CDUP\n", getpid());

                //ftp_cdup(&res);
                //response_send(sockfd_accpt, &res);
                break;

            case NOOP:
                printf("[CLIENT_%d] NOOP\n", getpid());

                ftp_noop(&res);
                response_send(sockfd_accpt, &res);
                break;

            case QUIT:
                printf("[CLIENT_%d] QUIT\n", getpid());

                ftp_quit(&res);
                response_send(sockfd_accpt, &res);
                
                close(sockfd_accpt);
                exit(EXIT_SUCCESS);

            case HELP:
            default:
                printf("[CLIENT_%d] HELP\n", getpid());

                //ftp_help(&res);
                //response_send(sockfd_accpt, &res);
                break;
            }
        }
    }
}

int response_send (int sockfd, response_t * res)
{    
    int nSent;

    nSent = send(sockfd, (void*) res, sizeof(response_t), 0);

    if (nSent < 0 || nSent != sizeof(response_t)) {
        fprintf(stderr, "[CLIENT_%d] ", getpid());
        perror("send()");

        close(sockfd);
        exit(EXIT_FAILURE);
    }  
}