#ifndef MSG_H
#define MSG_H

#define ADDR_SIZE sizeof(struct sockaddr_un)
#define MSG_SIZE sizeof(msg_t)
#define MAX_BUF 1024

typedef struct {
    int type; // typ komunikatu
    int count; // ile było malych liter
    int offset; // przesunięcie
    char text[MAX_BUF]; // tekst komunikatu
} msg_t;

enum MSG_TYPE {
    MSG_TYPE_ENDCOM = 0,
    MSG_TYPE_ROPEN,
    MSG_TYPE_WOPEN,
    MSG_TYPE_READ,
    MSG_TYPE_APPEND,
    MSG_TYPE_CLOSE,

    MSG_TYPE_ENDCOM_ACCEPT = 10,
    MSG_TYPE_ROPEN_ACCEPT,
    MSG_TYPE_WOPEN_ACCEPT,
    MSG_TYPE_READ_ACCEPT,
    MSG_TYPE_APPEND_ACCEPT,
    MSG_TYPE_CLOSE_ACCEPT,

    MSG_TYPE_ERR = 21
};

#endif