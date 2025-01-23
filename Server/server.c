#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <wait.h>

#define ADDR_SIZE sizeof(struct sockaddr_un)
#define MSG_SIZE sizeof(msg_t)
#define MAX_BUF 1024

typedef struct {
    int typ; // typ komunikatu
    int ile; // ile było malych liter
    int off; // przesunięcie
    char text[MAX_BUF]; // tekst komunikatu
} msg_t;

enum MSG_TYPE {
    MSG_TYPE_ENDCOM = 0,
    MSG_TYPE_ROPEN,
    MSG_TYPE_WOPEN,
    MSG_TYPE_READ,
    MSG_TYPE_WRITE,

    MSG_TYPE_ENDCOM_ACCEPT = 10,
    MSG_TYPE_ROPEN_ACCEPT,
    MSG_TYPE_WOPEN_ACCEPT,
    MSG_TYPE_READ_ACCEPT,
    MSG_TYPE_WRITE_ACCEPT,

    MSG_TYPE_ERR = 21
};

void connection_loop(int sockfd_accpt);
static void errexit_if_equals(const int val, const int check_val, const char * errlabel);
static void errexit_if_smaller(const int val, const int check_val, const char * errlabel);

int main (int argc, char** argv) 
{
    int status;
    int sockfd,
        sockfd_accpt;
    struct sockaddr_un addr = { AF_UNIX, "/tmp/zad6.sock" };
    
    // CREATE AN ENDPOINT //////////////////////

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    errexit_if_equals(sockfd, -1, "socket");

    // BIND THE SOCKET TO ADDRESS //////////////
    
    status = bind(sockfd, (struct sockaddr*) &addr, ADDR_SIZE);
    errexit_if_equals(status, -1, "bind");

    // PREPARE TO ACCEPT CONNECTIONS ///////////

    status = listen(sockfd, 5);
    errexit_if_equals(status, -1, "listen");
    printf("Server listening...\n");

    // SERVER LOOP /////////////////////////////

    while(1) {
        // accept the connection and create a child process for it
        sockfd_accpt = accept(sockfd, NULL, NULL);
        errexit_if_equals(sockfd_accpt, -1, "accept");

        if (fork() == 0) {
            connection_loop(sockfd_accpt);
        }
    }
}

void connection_loop(int sockfd_accpt)
{
    printf("%d: Connection accepted.\n", getpid());

    int count;
    msg_t packet;

    while (1) {
        // Receive the message
        count = recv(sockfd_accpt, (msg_t*) &packet, MSG_SIZE, 0);

        // PACKET TYPE LOGIC ////////////////////////////////////

        if (packet.typ == MSG_TYPE_ENDCOM) {
            printf("%d: Communication terminated. Exiting...\n", getpid());

            // Send a response accepting communication end
            count = send(sockfd_accpt, (void*) &packet, MSG_SIZE, 0);

            close(sockfd_accpt);

            exit(EXIT_SUCCESS);
        }
    }
}

// Exit if 'val' equals 'check_val'
//
static void errexit_if_equals(const int val, const int check_val, const char * errlabel)
{
    if (val == check_val) { 
        perror(errlabel); 
        exit (EXIT_FAILURE); 
    }
}

// Exit if 'val' is smallet than 'check_val'
//
static void errexit_if_smaller(const int val, const int check_val, const char * errlabel)
{
    if (val < check_val) { 
        perror(errlabel); 
        exit (EXIT_FAILURE); 
    }
}