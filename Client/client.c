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
static int data_pasv_socket_create(char * hostname, char * port);
static int command_get(command_t * cmd);

typedef struct ipv4_addr {
    int p1;
    int p2;
    int p3;
    int p4;
} ipv4_addr_t;

typedef struct port {
    int p1;
    int p2;
} port_t;


int main(int argc, char** argv)
{
    int nSent, nRead;
    int sockfd, sockfd_data;

    if (argc != 2) {
        fprintf(stderr, "Usage: ./client HOSTNAME\n");
        exit(EXIT_FAILURE);
    }

    // CREATE SOCKET

    sockfd = conn_socket_create(argv[1]);

    // CLIENT LOOP

    command_t cmd;
    response_t res;
    char buffer[BUFFER_SIZE];

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

        if (nRead <= 0) {
            fprintf(stderr, "<ERR> ");
            perror("recv()");

            close(sockfd);
            close(sockfd_data);
            exit(EXIT_FAILURE);
        }

        // TODO: Print reply
        printf("%d, %s\n", res.code, res.message);

        switch (cmd.type) {
        case PASV:
            if(res.code != 227)
                break;

            // ESTABLISH DATA CONNECTION

            ipv4_addr_t ipv4;
            port_t data_port;

            char ipv4_str[100];
            char data_port_str[10];

            sscanf(res.message, "Entering passive mode. %d,%d,%d,%d,%d,%d",
                &ipv4.p1, &ipv4.p2, &ipv4.p3, &ipv4.p4, &data_port.p1, &data_port.p2);

            snprintf(ipv4_str, 100, "%d.%d.%d.%d", ipv4.p1, ipv4.p2, ipv4.p3, ipv4.p4);
            snprintf(data_port_str, 10, "%d", data_port.p1 * 256 + data_port.p2);

            sockfd_data = data_pasv_socket_create(ipv4_str, data_port_str);

            break;

        case LIST:
            if(res.code != 150)
                break;

            // RECEIVE DATA

            while((nRead = recv(sockfd_data, (char *) &buffer, sizeof(buffer), 0)) > 0) {
                if(nRead < 0) {
                    fprintf(stderr, "<ERR> ");
                    perror("recv()");

                    close(sockfd_data);
                    break;
                }

                printf(buffer);
            }
            
            // GRACEFULLY CLOSE CONNECTION
            
            if((nRead = recv(sockfd, (response_t *) &res, sizeof(response_t), 0)) < 0) {
                fprintf(stderr, "<ERR> ");
                perror("recv()");

                close(sockfd);
                close(sockfd_data);
                exit(EXIT_FAILURE);
            }

            printf("%d, %s\n", res.code, res.message);

            close(sockfd_data);
            break;

        case RETR:
            if(res.code != 150)
                break;

            // CREATE (OR OVERWRITE) FILE
            FILE * fptr;

            fptr = fopen(cmd.args, "w");

            // RECEIVE DATA

            while((nRead = recv(sockfd_data, (char *) &buffer, sizeof(buffer), 0)) > 0) {
                if(nRead < 0) {
                    fprintf(stderr, "<ERR> ");
                    perror("recv()");

                    fclose(fptr);
                    close(sockfd_data);
                    break;
                }

                fwrite(buffer, sizeof(buffer), 1, fptr);

                if (ferror(fptr)) {
                    fprintf(stderr, "<ERR> ");
                    perror("fwrite()");

                    fclose(fptr);
                    close(sockfd_data);
                    break;
                }
            }
            
            // GRACEFULLY CLOSE CONNECTION
            
            if((nRead = recv(sockfd, (response_t *) &res, sizeof(response_t), 0)) < 0) {
                fprintf(stderr, "<ERR> ");
                perror("recv()");

                fclose(fptr);
                close(sockfd);
                close(sockfd_data);
                exit(EXIT_FAILURE);
            }

            printf("%d, %s\n", res.code, res.message);

            fclose(fptr);
            close(sockfd_data);
            break;
        }

        // Quit
        if (res.code == 221 || res.code == 421) {
            close(sockfd);
            close(sockfd_data);
            exit(EXIT_SUCCESS);
        }
    }
}


static int command_get(command_t * cmd) {
    char buffer[BUFFER_SIZE + 4]; // +4 to account for type
    char cmdTypeRaw[5];
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

    if(strlen(cmdTypeRaw) > 4) {
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
        cmd->args[0] = '\0';
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
        cmd->args[0] = '\0';
    } else if (strncmp(cmdTypeRaw, "CWD", 3) == 0) {
        cmd->type = CWD;
    } else if (strncmp(cmdTypeRaw, "CDUP", 4) == 0) {
        cmd->type = CDUP;
        cmd->args[0] = '\0';
    } else if (strncmp(cmdTypeRaw, "PASS", 4) == 0) {
        cmd->type = PASS;
    } else if (strncmp(cmdTypeRaw, "USER", 4) == 0) {
        cmd->type = USER;
    } else if (strncmp(cmdTypeRaw, "NOOP", 4) == 0) {
        cmd->type = NOOP;
        cmd->args[0] = '\0';
    } else if (strncmp(cmdTypeRaw, "QUIT", 4) == 0) {
        cmd->type = QUIT;
        cmd->args[0] = '\0';
    } else if (strncmp(cmdTypeRaw, "PASV", 4) == 0) {
        cmd->type = PASV;
        cmd->args[0] = '\0';
    } else if (strncmp(cmdTypeRaw, "PORT", 4) == 0) {
        cmd->type = PORT;
    } else {
        return -1;
    }

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
    printf("Client connected on %s:%s...\n", ipstr, CONN_PORT);

    freeaddrinfo(res);

    return sockfd;
}

static int data_pasv_socket_create(char * hostname, char * port) {
    int sockfd;
    struct addrinfo hints, * res, * p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP

    if (getaddrinfo(hostname, port, &hints, &res) != 0) {
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
    printf("Client connected on %s:%s...\n", ipstr, port);

    freeaddrinfo(res);

    return sockfd;
}