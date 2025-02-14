#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../Common/exits.h"
#include "../Common/msg.h"

int get_type(void);
void assign_text(char* buf);

int main (int argc, char** argv)
{
    int status;
    int sockfd;
    struct sockaddr_un addr = { AF_UNIX, "/tmp/zad6.sock" };

    // CREATE AN ENDPOINT //////////////////////

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    errexit_if_equals(sockfd, -1, "socket");

    // CONNECT TO THE SOCKET ///////////////////

    status = connect(sockfd, (struct sockaddr*) &addr, ADDR_SIZE);
    errexit_if_equals(status, -1, "connect");

    // SEND A MESSAGE //////////////////////////

    int count;
    msg_t packet;

    while (1) {
        // Prepare the message
        memset(&packet, 0, sizeof(packet));  // Clear the structure

        packet.type = get_type();        
        if (packet.type != MSG_TYPE_ENDCOM) assign_text(packet.text);

        // Send and receive the message
        count = send(sockfd, (void*) &packet, MSG_SIZE, 0);        
        count = recv(sockfd, (msg_t*) &packet, MSG_SIZE, 0);

        // PACKET TYPE LOGIC ////////////////////////////////////

        switch (packet.type) {
            case MSG_TYPE_ENDCOM_ACCEPT:
                printf("Communication terminated. Exiting...\n");

                close(sockfd);
                exit(EXIT_SUCCESS);
            
            case MSG_TYPE_ROPEN_ACCEPT:
                
                break;
            
            case MSG_TYPE_WOPEN_ACCEPT:
                
                break;
            
            case MSG_TYPE_READ_ACCEPT:
                
                break;
            
            case MSG_TYPE_APPEND_ACCEPT:
                
                break;
            
            case MSG_TYPE_CLOSE_ACCEPT:
                
                break;
        }
    }
}

// Get a number from the user
//
int get_type(void) {
    while (1)
    {
        printf("Select the message type:\n");
        printf("\t0 -- End communications\n");
        printf("\t1 -- Open file with read privileges\n");
        printf("\t2 -- Open file with write privileges\n");
        printf("\t4 -- Read from file\n");
        printf("\t3 -- Append to file\n");
        printf("\t5 -- Close file\n");

        // Read character
        char type = getchar();
    
        // Clear buffer of leftover characters
        char c;
        while ( (c = getchar()) != '\n' && c != EOF ) { }

        switch(type) {
            case '0':
                return MSG_TYPE_ENDCOM;
            case '1':
                return MSG_TYPE_ROPEN;
            case '2':
                return MSG_TYPE_WOPEN;
            case '3':
                return MSG_TYPE_READ;
            case '4':
                return MSG_TYPE_APPEND;
            case '5':
                return MSG_TYPE_CLOSE;
            default:
                printf("Incorrect type! Try again.\n\n");
        }
    }
}

// Get a string of user-typed characters and assign it to buffer
void assign_text(char* buf) {
    printf("Type the message: ");

    // Read into buf
    fgets(buf, MAX_BUF, stdin);
}