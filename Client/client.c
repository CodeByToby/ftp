#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h> // (addrinfo)
#include <arpa/inet.h> // inet_ntop

#include "../Common/packets.h"
#include "../Common/command_types.h"
#include "../Common/defines.h"
#include "../Common/string_helpers.h"

#define CONN_PORT "3456"

static int conn_socket_create(char * hostname);
static int command_get(command_t * cmd);

int main(int argc, char** argv)
{
    int nSent, nRead;
    int sockfd;

    if (argc != 2) {
        fprintf(stderr, "Usage: ./client HOSTNAME\n");
        exit(EXIT_FAILURE);
    }

    // CREATE SOCKET

    sockfd = conn_socket_create(argv[1]);

    // CLIENT LOOP

    command_t cmd;
    response_t res;

    while(TRUE) {
        memset(&cmd, 0, sizeof(cmd));
        memset(&res, 0, sizeof(res));

        printf("> ");
		while(command_get(&cmd) < 0) {
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

        // Quit
        if (res.code == 221 || res.code == 421) {
            close(sockfd);
            exit(EXIT_SUCCESS);
        }
    }
}

static int command_get(command_t * cmd) {
    char buffer[BUFFER_SIZE + 4]; // +4 to account for type
    char cmdTypeRaw[4];
    char *token;

    memset(&buffer, 0, sizeof(buffer));
    memset(&cmdTypeRaw, 0, sizeof(cmdTypeRaw));

    // GET INPUT

    fgets(buffer, sizeof(buffer), stdin);

    // ... Type
    token = strtok(buffer, " ");
    if (token != NULL) {
        trim(token);
        strncpy(cmdTypeRaw, token, sizeof(cmdTypeRaw));
    } else {
        return -1;
    }

    for (int i = 0; i < sizeof(cmdTypeRaw); ++i) {
        cmdTypeRaw[i] = toupper(cmdTypeRaw[i]);
    }

    // ... Args
    token = strtok(NULL, "\n");
    if (token != NULL) {
        trim(token);
        strncpy(cmd->args, token, sizeof(cmd->args));
    } else {
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

static int conn_socket_create(char * hostname) {
    int sockfd;
    struct addrinfo hints, * res, * p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    if (getaddrinfo(hostname, CONN_PORT, &hints, &res) != 0) {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }

    for(p = res; p != NULL; p = p->ai_next) {

        // CREATE AN ENDPOINT

        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) { 
            perror("socket()"); 
            continue;
        }

        // CONNECT TO THE SOCKET

        if(connect(sockfd, p->ai_addr, p->ai_addrlen) < 0) { 
            perror("connect()"); 
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Client failed to connect\n");
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }

    char ipstr[INET_ADDRSTRLEN];
    struct sockaddr_in *ip = (struct sockaddr_in *) p->ai_addr;

    inet_ntop(p->ai_family, &(ip->sin_addr), ipstr, sizeof(ipstr));
    printf("Server running on %s:%s...\n", ipstr, CONN_PORT);

    freeaddrinfo(res);

    return sockfd;
}