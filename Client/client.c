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

#define IS_CLIENT

#include "../Common/packets.h"
#include "../Common/command_types.h"
#include "../Common/defines.h"
#include "../Common/string_helpers.h"

#define CONN_PORT "3456"

static int conn_socket_create(char * hostname);
static int data_pasv_socket_create(char * hostname, char * port);

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
		while(command_set(&cmd) < 0) {
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

            while((nRead = recv(sockfd_data, (char *) buffer, sizeof(buffer), 0)) > 0) {
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

            FILE * fptr_retr;
            fptr_retr = fopen(cmd.args, "w");

                // RECEIVE DATA

                sleep(1); // Hacky way of preventing recv before server sends data

                while((nRead = recv(sockfd_data, (char *) buffer, sizeof(buffer) - 1, 0)) > 0) {
                    fwrite(buffer, strlen(buffer), 1, fptr_retr);

                    if (ferror(fptr_retr)) {
                        fprintf(stderr, "<ERR> ");
                        perror("fwrite()");

                        break;
                    }
                }

                shutdown(sockfd_data, SHUT_RD);
            
            fclose(fptr_retr);
            close(sockfd_data);

            if(nRead < 0) {
                fprintf(stderr, "<ERR> ");
                perror("recv()");

                break;
            }

            // GRACEFULLY CLOSE CONNECTION
            
            if((nRead = recv(sockfd, (response_t *) &res, sizeof(response_t), 0)) < 0) {
                fprintf(stderr, "<ERR> ");
                perror("recv()");

                close(sockfd);
                exit(EXIT_FAILURE);
            }

            printf("%d, %s\n", res.code, res.message);

            break;

        case STOR:
            if(res.code != 150)
                break;

            // OPEN FILE FOR READING

            FILE * fptr_stor;
            fptr_stor = fopen(cmd.args, "r");

                // SEND DATA

                while(feof(fptr_stor) == 0) {
                    memset(buffer, 0, sizeof(buffer));

                    fread(buffer, sizeof(buffer) - 1, 1, fptr_stor);

                    if(ferror(fptr_stor) != 0) {
                        fprintf(stderr, "<ERR> ");
                        perror("fread()");

                        break;
                    }

                    if(buffer[0] != '\0') {
                        int nSent = send(sockfd_data, (void *) buffer, sizeof(buffer) - 1, 0);

                        if(nSent < 0 || nSent != sizeof(buffer) - 1) {
                            fprintf(stderr, "<ERR> ");
                            perror("send()");

                            fclose(fptr_stor);
                            close(sockfd);
                            close(sockfd_data);
                            exit(EXIT_FAILURE);
                        }
                    }
                }

                shutdown(sockfd_data, SHUT_WR);

            fclose(fptr_stor);
            close(sockfd_data);

            // GRACEFULLY CLOSE CONNECTION
            
            if((nRead = recv(sockfd, (response_t *) &res, sizeof(response_t), 0)) < 0) {
                fprintf(stderr, "<ERR> ");
                perror("recv()");

                close(sockfd);
                close(sockfd_data);
                exit(EXIT_FAILURE);
            }

            printf("%d, %s\n", res.code, res.message);
            
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