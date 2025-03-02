#ifndef SOCKET_H
#define SOCKET_H

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

int ipv4_get(ipv4_addr_t * ip);
int port_get(port_t * port, char * port_str, size_t len);

int socket_create(const char * port, const int secs);
// TODO: PORT socket creation (no binding or listening)

int set_timeout(const int sockfd, const int secs);
int set_addrreuse(const int sockfd);

#endif