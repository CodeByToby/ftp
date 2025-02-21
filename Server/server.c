#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h> setpgid
#include <ctype.h>
#include <string.h>
#include <sys/stat.h> // mkdir
#include <errno.h> // errno
#include <sys/time.h> // timeval
#include <signal.h> // signal killpg
#include <sys/wait.h> // waitpid

#include "../Common/packets.h"
#include "../Common/defines.h"
#include "tstamp.h"
#include "handlers.h"

#define PRNT_TIMEOUT 180
#define CHLD_TIMEOUT 60

static int response_send(int sockfd, response_t * res);
static int child_process_logic(int sockfd_accpt);

volatile int isClosing = FALSE;
volatile int nChildren = 0;

void sigint_handler(int signo) { isClosing = TRUE; }
void sigchld_handler(int signo) {
    nChildren--;

    // Taken from https://beej.us/guide/bgnet/
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

int main(int argc, char** argv) 
{
    int chldpid;
    int sockfd,
        sockfd_accpt;
    const struct sockaddr_un addr = { AF_UNIX, "/tmp/ftp.sock" };

    // CREATE AN ENDPOINT

    if((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) { 
        perror("socket()"); 
        exit(EXIT_FAILURE); 
    }

    // DEFINE TIMEOUT FOR MAIN SOCKET

    struct timeval tv;
    tv.tv_sec = PRNT_TIMEOUT;
    tv.tv_usec = 0;
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0) {
        perror("setsockopt()"); 
        exit(EXIT_FAILURE);
    }

    // BIND THE SOCKET TO ADDRESS

    if(bind(sockfd, (struct sockaddr*) &addr, sizeof(addr)) < 0) { 
        perror("bind()"); 
        exit(EXIT_FAILURE);
    }

    // PREPARE TO ACCEPT CONNECTIONS

    if(listen(sockfd, 5) < 0) { 
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

    // SERVER SESSION

    int pgid; 
    int isFirstProcess = TRUE;

    // SIGNAL HANDLING

    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);

    while(1) {
        if((sockfd_accpt = accept(sockfd, NULL, NULL)) < 0) {
            close(sockfd);
            killpg(pgid, SIGINT);

            while(nChildren != 0);

            if(errno == ETIMEDOUT || errno == EINTR) {
                tstamp(stdout);
                printf(" - [SERVER / INFO] - Process has timed out or has been interrupted\n");

                exit(EXIT_SUCCESS);
            } else {
                tstamp(stderr);
                fprintf(stderr, " - [SERVER / ERRO] - ");
                perror("accept()");
                
                exit(EXIT_FAILURE);
            }
        }

        // FORK IN THE ROAD

        chldpid = fork();

        if(chldpid < 0) {
            fprintf(stderr, "<ERR> ");
            perror("fork()"); 

            close(sockfd_accpt);
        } 
        if(chldpid == 0) {
            close(sockfd); // Child process does not need this
            child_process_logic(sockfd_accpt);
        }

        nChildren++;
        close(sockfd_accpt); // Parent process does not need this

        // Prepare a process group for all children
        if(isFirstProcess == TRUE) {
            isFirstProcess = FALSE;
            pgid = chldpid;
        }
        
        setpgid(chldpid, pgid);
    }
}

static int child_process_logic(int sockfd_accpt) {
    int nRead;

    command_t cmd;
    response_t res;

    // DEFINE TIMEOUT FOR ACCEPT SOCKET

    struct timeval tv;
    tv.tv_sec = CHLD_TIMEOUT;
    tv.tv_usec = 0;
    if(setsockopt(sockfd_accpt, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0) {
        tstamp(stderr);
        fprintf(stderr, " - [CLIENT_%d / ERRO] - ", getpid());
        perror("setsockopt()"); 

        exit(EXIT_FAILURE); 
    }

    // CLIENT-SPECIFIC SESSION

    user_session_t session;

    memset(&session, 0, sizeof(session));
    session.state = LOGGED_OUT;

    while(TRUE) {
        memset(&cmd, 0, sizeof(cmd));
        memset(&res, 0, sizeof(res));

        if(isClosing == TRUE || (nRead = recv(sockfd_accpt, (command_t*) &cmd, sizeof(command_t), 0)) < 0) {
            response_set(&res, 421, "Service not available, closing control connection");
            send(sockfd_accpt, (void*) &res, sizeof(response_t), 0);

            close(sockfd_accpt);

            if(errno == ETIMEDOUT || errno == EINTR) {
                tstamp(stdout);
                printf(" - [CLIENT_%d / INFO] - Process has timed out or has been interrupted\n", getpid());

                exit(EXIT_SUCCESS);
            } else {
                tstamp(stderr);
                fprintf(stderr, " - [CLIENT_%d / ERRO] - ", getpid());
                perror("recv()"); 

                exit(EXIT_FAILURE);
            }
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

static int response_send(int sockfd, response_t * res)
{    
    int nSent = send(sockfd, (void*) res, sizeof(response_t), 0);

    if(nSent < 0 || nSent != sizeof(response_t)) {
        tstamp(stderr);
        fprintf(stderr, " - [CLIENT_%d / ERRO] - ", getpid());
        perror("send()");

        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return 0;
}