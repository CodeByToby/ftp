#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include <wait.h>
#include <semaphore.h>

#include "../Common/exits.h"
#include "../Common/msg.h"

void connection_loop(int sockfd_accpt);

int /* fd */ PrepareSocket(const struct sockaddr_un *addr);

int main (int argc, char** argv) 
{
    int status;
    int sockfd,
        sockfd_accpt;
    const struct sockaddr_un addr = { AF_UNIX, "/tmp/zad6.sock" };

    sockfd = PrepareSocket(&addr);

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

int /* fd */ PrepareSocket(const struct sockaddr_un *addr) {
    int status;
    int sockfd;
    
    // CREATE AN ENDPOINT //////////////////////
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    errexit_if_equals(sockfd, -1, "socket");

    // BIND THE SOCKET TO ADDRESS /////////////
    status = bind(sockfd, (struct sockaddr*) &addr, ADDR_SIZE);
    errexit_if_equals(status, -1, "bind");

    // PREPARE TO ACCEPT CONNECTIONS ///////////

    status = listen(sockfd, 5);
    errexit_if_equals(status, -1, "listen");
    printf("Server listening...\n");
    
    return sockfd;
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
        switch (packet.type) 
        {
        case MSG_TYPE_ENDCOM:
            printf("%d: Communication terminated. Exiting...\n", getpid());

            // Send a response accepting communication end
            count = send(sockfd_accpt, (void*) &packet, MSG_SIZE, 0);

            close(sockfd_accpt);

            exit(EXIT_SUCCESS);
        
        case MSG_TYPE_ROPEN:
            break;
        
        case MSG_TYPE_WOPEN:
            
            break;
        
        case MSG_TYPE_READ:
            
            break;
        
        case MSG_TYPE_APPEND:
            
            break;
        
        case MSG_TYPE_CLOSE:
            
            break;
        }
    }
}