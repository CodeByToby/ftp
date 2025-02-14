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

int main (int argc, char** argv) 
{
    int nSent, nRead;

    int status;
    int sockfd,
        sockfd_accpt;
    const struct sockaddr_un addr = { AF_UNIX, "/tmp/ftp.sock" };

    // CREATE AN ENDPOINT

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) { 
        perror("socket()"); 
        exit(EXIT_FAILURE); 
    }

    // DEFINE TIMEOUT FOR SOCKET

    struct timeval tv;
    tv.tv_sec = 60;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    // BIND THE SOCKET TO ADDRESS

    if ((status = bind(sockfd, (struct sockaddr*) &addr, sizeof(addr))) < 0) { 
        perror("bind()"); 
        exit(EXIT_FAILURE);
    }

    // PREPARE TO ACCEPT CONNECTIONS

    if ((status = listen(sockfd, 5)) < 0) { 
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

        // FORK LOOP

        if (fork() == 0) {
            while (TRUE) {
                nRead = recv(sockfd_accpt, (command_t*) &cmd, sizeof(command_t), 0);

                if (nRead < 0) {
                    fprintf(stderr, "<ERR> PID_%d: ", getpid());
                    perror("recv()");

                    close(sockfd);
                    exit(EXIT_FAILURE);
                }

                switch (cmd.type) {
                case LIST:
                    ftp_list(sockfd_accpt, &res);
                    printf("[CLIENT_%d] LIST\n", getpid());
                    break;

                case RETR:
                    //ftp_retr(sockfd_accpt, &res);
                    printf("[CLIENT_%d] RETR\n", getpid());
                    break;

                case STOR:
                    //ftp_stor(sockfd_accpt, &res);
                    printf("[CLIENT_%d] STOR\n", getpid());
                    break;

                case STOU:
                    //ftp_stou(sockfd_accpt, &res);
                    printf("[CLIENT_%d] STOU\n", getpid());
                    break;

                case APPE:
                    //ftp_appe(sockfd_accpt, &res);
                    printf("[CLIENT_%d] APPE\n", getpid());
                    break;

                case DELE:
                    //ftp_dele(sockfd_accpt, &res);
                    printf("[CLIENT_%d] DELE\n", getpid());
                    break;

                case RMD:
                    //ftp_rmd(sockfd_accpt, &res);
                    printf("[CLIENT_%d] RMD\n", getpid());
                    break;

                case MKD:
                    //ftp_mkd(sockfd_accpt, &res);
                    printf("[CLIENT_%d] MKD\n", getpid());
                    break;

                case PWD:
                    //ftp_pwd(sockfd_accpt, &res);
                    printf("[CLIENT_%d] PWD\n", getpid());
                    break;

                case CWD:
                    //ftp_cwd(sockfd_accpt, &res);
                    printf("[CLIENT_%d] CWD\n", getpid());
                    break;

                case CDUP:
                    //ftp_cdup(sockfd_accpt, &res);
                    printf("[CLIENT_%d] CDUP\n", getpid());
                    break;

                case NOOP:
                    ftp_noop(sockfd_accpt, &res);
                    printf("[CLIENT_%d] NOOP\n", getpid());
                    break;

                case QUIT:
                    //ftp_quit(sockfd_accpt, &res);
                    printf("[CLIENT_%d] QUIT\n", getpid());
                    break;

                case HELP:
                default:
                    //ftp_help(sockfd_accpt, &res);
                    printf("[CLIENT_%d] HELP\n", getpid());
                    break;
                }
            }
        }
    }
}