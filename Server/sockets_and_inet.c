#include <stdio.h> // perror (stderr) sscanf rand
#include <stdlib.h> // (NULL)
#include <unistd.h> // close gethostname
#include <string.h> // memset
#include <netdb.h> // (addrinfo)
#include <sys/time.h> // timeval
#include <sys/socket.h> // socket setsockopt getaddrinfo
#include <arpa/inet.h> // inet_ntoa

#include "sockets_and_inet.h"

/// @brief Get, parse and copy the ip into a ipv4_addr_t struct variable.
/// @param ip_out The ipv4_addr_t struct to be filled.
/// @return 0 if succesfully parsed and copied, and -1 for errors.
int ipv4_get(ipv4_addr_t * ip_out) {
    char host[256];
    struct addrinfo hints, * res;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = 0;

    if(gethostname(host, sizeof(host)) < 0) {
        printf(host);
        perror("gehostname()");
        return -1;
    }

    if(getaddrinfo(host, NULL, &hints, &res) < 0) {
        perror("getaddrinfo()");
        return -1;
    }

    // FORMAT IP TO STRUCT

    char * ipstr;
    struct sockaddr_in * ip;

    ip = (struct sockaddr_in *) res->ai_addr;
    ipstr = inet_ntoa(ip->sin_addr);

    sscanf(ipstr, "%d.%d.%d.%d", &ip_out->p1, &ip_out->p2, &ip_out->p3, &ip_out->p4);

    freeaddrinfo(res); 
    return 0;
}

/// @brief Randomly generate a port and copy it into `port` and `port_str`.
/// @param port The port_t struct to be filled.
/// @param port_str The string to be filled.
/// @param len Length of `port_str`.
/// @return Always 0.
int port_get(port_t * port, char * port_str, size_t len) {
    // Values between 1024 and 65535
    int pmin = 1024, pmax = 65535;
    int p = (rand() % (pmax - pmin + 1)) + pmin;

    port->p1 = p / 256;
    port->p2 = p % 256;

    snprintf(port_str, len, "%d", p);

    return 0;
}

/// @brief Create a socket.
/// @param port Port number of the socket.
/// @param secs Timeout of the socket in seconds.
/// @return sockfd of the new socket, or -1 if socket could not be created.
int socket_create(const char * port, const int secs) {
    int sockfd;
    struct addrinfo hints, * res, * p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    if(getaddrinfo(NULL, port, &hints, &res) != 0) {
        perror("getaddrinfo()");
        return -1;
    }

    for(p = res; p != NULL; p = p->ai_next) {

        // CREATE AN ENDPOINT

        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
            continue;
        }

        // SET SOCKET OPTIONS

        // Time out
        if(set_timeout(sockfd, secs) < 0) {
            close(sockfd);
            continue;
        }

        // Reusing address
        if(set_addrreuse(sockfd) < 0) {
            close(sockfd);
            continue;
        }

        // BIND THE SOCKET TO ADDRESS

        if(bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) { 
            close(sockfd);
            continue;
        }

        // PREPARE TO ACCEPT CONNECTIONS

        if(listen(sockfd, 10) < 0) { 
            close(sockfd);
            continue;
        }

        break;
    }

    freeaddrinfo(res); // No longer needed

    if(p == NULL) {
        return -1;
    }

    return sockfd;
}


/// @brief Set a timeout option on the socket.
/// @param sockfd The file descriptor of the socket.
/// @param secs Timeout in seconds, -1 for no timeout.
/// @return setsockopt return values.
int set_timeout(const int sockfd, const int secs) {
    struct timeval tv;
    tv.tv_sec = secs;
    tv.tv_usec = 0;
    
    return setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
}

/// @brief Set a resuable address option on the socket.
/// @param sockfd The file descriptor of the socket.
/// @return setsockopt return values.
int set_addrreuse(const int sockfd) {
    int yes = 1;

    return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
}