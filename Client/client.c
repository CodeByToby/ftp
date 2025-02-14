#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../Common/packets.h"
#include "../Common/defines.h"

int getcommand(command_t * cmd);

int main(int argc, char** argv)
{
    int nSent, nRead;

    int retval;
    int sockfd;
    struct sockaddr_un addr = { AF_UNIX, "/tmp/ftp.sock" };

    // CREATE AN ENDPOINT

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) { 
        perror("socket()"); 
        exit(EXIT_FAILURE); 
    }

    // CONNECT TO THE SOCKET

    if ((retval = connect(sockfd, (struct sockaddr*) &addr, sizeof(addr))) < 0) {
        perror("connect()");
        exit(EXIT_FAILURE); 
    }

    // CLIENT LOOP

    command_t cmd;
    response_t res;

    while(TRUE) {
        memset(&cmd, 0, sizeof(cmd));
        memset(&res, 0, sizeof(res));

        printf("> ");
		if(getcommand(&cmd) < 0) {
            printf("Invalid command. Try again\n");
			continue;
        }

        // Send command
        nSent = send(sockfd, (void*) &cmd, sizeof(command_t), 0);

        if (nSent < 0 || nSent != sizeof(command_t)) {            
            fprintf(stderr, "<ERR> ");
            perror("send()");

            close(sockfd);
            exit(EXIT_FAILURE);
        }
        
        // Receive reply
        nRead = recv(sockfd, (response_t *) &res, sizeof(response_t), 0);

        if (nRead < 0) {
            fprintf(stderr, "<ERR> ");
            perror("recv()");

            close(sockfd);
            exit(EXIT_FAILURE);
        }

        // TODO: Print reply
        printf("%d, %s\n", res.code, res.message);
        usleep(1000000);
    }
}

int getcommand(command_t * cmd) {
    // TODO: Handle user input
    
    cmd->type = LIST;

    return 0;
}