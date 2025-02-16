#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include "../Common/packets.h"
#include "../Common/defines.h"

int getcommand(command_t * cmd);

int main(int argc, char** argv)
{
    int nSent, nRead;

    int sockfd;
    struct sockaddr_un addr = { AF_UNIX, "/tmp/ftp.sock" };

    // CREATE AN ENDPOINT

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) { 
        perror("socket()"); 
        exit(EXIT_FAILURE); 
    }

    // CONNECT TO THE SOCKET

    if (connect(sockfd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
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
		while(getcommand(&cmd) < 0) {
            printf("Invalid command. Try again\n> ");
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
    char buffer[BUFFER_SIZE + 4]; // +4 to account for type
    char cmdTypeRaw[4];
    char *token;

    memset(&buffer, 0, sizeof(buffer));
    memset(&cmdTypeRaw, 0, sizeof(cmdTypeRaw));

    // GET INPUT

    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0; // Get rid of newline

    token = strtok(buffer, " ");
    if (token != NULL) {
        strncpy(cmdTypeRaw, token, sizeof(cmdTypeRaw));
    } else { // If no command was provided
        return -1;
    }

    token = strtok(NULL, "");
    if (token != NULL) {
        strncpy(cmd->args, token, sizeof(cmd->args) - 1);
        cmd->args[sizeof(cmd->args) - 1] = '\0';
    } else { // If command was provided without any arguments
        cmd->args[0] = '\0';
    }

    // PARSE COMMAND TYPE

    if (strncmp(cmdTypeRaw, "LIST", 4) == 0) {
        cmd->type = LIST;
    } else if (strncmp(cmdTypeRaw, "HELP", 4) == 0) {
        cmd->type = HELP;
    } else if (strncmp(cmdTypeRaw, "RETR", 4) == 0) {
        cmd->type = RETR;
    } else if (strncmp(cmdTypeRaw, "STOR", 4) == 0) {
        cmd->type = STOR;
    } else if (strncmp(cmdTypeRaw, "STOU", 4) == 0) {
        cmd->type = STOU;
    } else if (strncmp(cmdTypeRaw, "APPE", 4) == 0) {
        cmd->type = APPE;
    } else if (strncmp(cmdTypeRaw, "DELE", 4) == 0) {
        cmd->type = DELE;
    } else if (strncmp(cmdTypeRaw, "RMD", 3) == 0) {
        cmd->type = RMD;
    } else if (strncmp(cmdTypeRaw, "MKD", 3) == 0) {
        cmd->type = MKD;
    } else if (strncmp(cmdTypeRaw, "PWD", 3) == 0) {
        cmd->type = PWD;
    } else if (strncmp(cmdTypeRaw, "CWD", 3) == 0) {
        cmd->type = CWD;
    } else if (strncmp(cmdTypeRaw, "CDUP", 4) == 0) {
        cmd->type = CDUP;
    } else if (strncmp(cmdTypeRaw, "PASS", 4) == 0) {
        cmd->type = PASS;
    } else if (strncmp(cmdTypeRaw, "USER", 4) == 0) {
        cmd->type = USER;
    } else if (strncmp(cmdTypeRaw, "NOOP", 4) == 0) {
        cmd->type = NOOP;
    } else if (strncmp(cmdTypeRaw, "QUIT", 4) == 0) {
        cmd->type = QUIT;
    } else {
        return -1;
    }

    //printf("Type: %d\n", cmd->type);
    //printf("Arguments: %s\n", cmd->args);

    return 0;
}