#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h> // getpid setpgid
#include <ctype.h>
#include <string.h>
#include <sys/stat.h> // mkdir
#include <errno.h> // errno
#include <sys/time.h> // timeval
#include <signal.h> // signal killpg

#include "../Common/packets.h"
#include "../Common/command_types.h"
#include "../Common/defines.h"
#include "commands.h"
#include "signals.h"
#include "user_lock.h"
#include "log.h"

#define PRNT_TIMEOUT 180
#define CHLD_TIMEOUT 60

static int response_send(int sockfd, response_t * res);
static int child_process_logic(int sockfd_accpt);

volatile int isClosing = FALSE;

int main(int argc, char** argv) 
{
    int chldpid;
    int sockfd,
        sockfd_accpt;
    const struct sockaddr_un addr = { AF_UNIX, "/tmp/ftp.sock" };
    user_lock_array_t locks;

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
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // BIND THE SOCKET TO ADDRESS

    if(bind(sockfd, (struct sockaddr*) &addr, sizeof(addr)) < 0) { 
        perror("bind()"); 
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // PREPARE TO ACCEPT CONNECTIONS

    if(listen(sockfd, 5) < 0) { 
        perror("listen()"); 
        close(sockfd);
        exit(EXIT_FAILURE); 
    }

    // MAKE A HOME DIRECTORY FOR CLIENTS

    if(mkdir("home", 0700) && errno != EEXIST) {
        perror("mkdir()");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // CAP NR OF CONNECTIONS PER USER

    create_user_locks(&locks, sockfd, 1);

    // SERVER SESSION

    log_info("Online", 0);

    int pgid; 
    int isFirstProcess = TRUE;

    // SIGNAL HANDLING

    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);

    while(1) {
        if((sockfd_accpt = accept(sockfd, NULL, NULL)) < 0) {
            if(errno == ETIMEDOUT || errno == EINTR) {
                if(isClosing == FALSE) // SIGCHLD has been intercepted, don't close
                    continue;

                log_info("Process has timed out or has been interrupted. Closing all connections...", 0);
            } else {
                log_erro("accept()", 0);
            }
            
            killpg(pgid, SIGINT); // Send SIGINT to all children
            while(wait(NULL) > 0); // Wait until all childen have exited

            log_info("All connections have been closed. Exiting", 0);

            close(sockfd);
            destroy_user_locks(&locks);
            exit(EXIT_FAILURE);
        }

        // FORK IN THE ROAD

        if((chldpid = fork()) < 0) {
            log_erro("fork()", 0);
            close(sockfd_accpt);
        }

        if(chldpid == 0) {
            close(sockfd); // Child process does not need this
            child_process_logic(sockfd_accpt);

            exit(EXIT_FAILURE); // fork should not reach this, hence FAILURE
        }

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
        log_erro("setsockopt()", getpid());
        exit(EXIT_FAILURE); 
    }

    // CLIENT-SPECIFIC SESSION

    user_session_t session;

    memset(&session, 0, sizeof(session));
    session.state = LOGGED_OUT;

    while(TRUE) {
        memset(&cmd, 0, sizeof(cmd));
        memset(&res, 0, sizeof(res));

        if((nRead = recv(sockfd_accpt, (command_t*) &cmd, sizeof(command_t), 0)) < 0 && isClosing == TRUE) {
            response_set(&res, 421, "Service not available, closing control connection");
            send(sockfd_accpt, (void*) &res, sizeof(response_t), 0);

            if(errno == ETIMEDOUT || errno == EINTR) {
                log_info("Process has timed out or has been interrupted", getpid());
            } else {
                log_erro("recv()", getpid());
            }

            close(sockfd_accpt);
            exit(EXIT_FAILURE);

        }

        switch (cmd.type) {
        case USER:
            log_comm("USER", getpid(), &cmd);
            ftp_user(&res, &cmd, &session);

            log_resp(getpid(), &res);
            response_send(sockfd_accpt, &res);
            break;
            
        case PASS:
            log_comm("PASS", getpid(), &cmd);
            ftp_pass(&res, &cmd, &session);

            log_resp(getpid(), &res);
            response_send(sockfd_accpt, &res);
            break;

        case LIST:
            log_comm("LIST", getpid(), &cmd);
            ftp_list(&res, &cmd, &session);

            log_resp(getpid(), &res);
            response_send(sockfd_accpt, &res);
            break;

        case RMD:
            log_comm("RMD", getpid(), &cmd);
            ftp_rmd(&res, &cmd, &session);

            log_resp(getpid(), &res);
            response_send(sockfd_accpt, &res);
            break;

        case MKD:
            log_comm("MKD", getpid(), &cmd);
            ftp_mkd(&res, &cmd, &session);

            log_resp(getpid(), &res);
            response_send(sockfd_accpt, &res);
            break;

        case PWD:
            log_comm("PWD", getpid(), &cmd);
            ftp_pwd(&res, &session);

            log_resp(getpid(), &res);
            response_send(sockfd_accpt, &res);
            break;

        case CWD:
            log_comm("CWD", getpid(), &cmd);
            ftp_cwd(&res, &cmd, &session);

            log_resp(getpid(), &res);
            response_send(sockfd_accpt, &res);
            break;

        case CDUP:
            log_comm("CDUP", getpid(), &cmd);
            strncpy(cmd.args, "..", 3);
            ftp_cwd(&res, &cmd, &session);

            log_resp(getpid(), &res);
            response_send(sockfd_accpt, &res);
            break;

        case NOOP:
            log_comm("NOOP", getpid(), &cmd);
            ftp_noop(&res);

            log_resp(getpid(), &res);
            response_send(sockfd_accpt, &res);
            break;

        case QUIT:
            log_comm("QUIT", getpid(), &cmd);
            ftp_quit(&res);

            log_resp(getpid(), &res);
            response_send(sockfd_accpt, &res);
            
            close(sockfd_accpt);
            exit(EXIT_SUCCESS);

        case HELP:
        default:
            log_comm("HELP", getpid(), &cmd);
            ftp_help(&res, &cmd);
            
            log_resp(getpid(), &res);
            response_send(sockfd_accpt, &res);
            break;
        }
    }
}

static int response_send(int sockfd, response_t * res)
{    
    int nSent = send(sockfd, (void*) res, sizeof(response_t), 0);

    if(nSent < 0 || nSent != sizeof(response_t)) {
        log_erro("send()", getpid());
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return 0;
}