#ifndef USER_LOCK_H
#define USER_LOCK_H

#include <semaphore.h> // sem_*

#include "../Common/packets.h"
#include "../Common/defines.h"

#define PER_USR_CLIENT_CAP 1

typedef struct user_lock {
    char username[BUFFER_SIZE];
    sem_t * lock;
} user_lock_t;

typedef struct user_lock_array {
    user_lock_t * arr;
    int size;
} user_lock_array_t;

void create_user_locks(user_lock_array_t * locks, int sockfd, int cap);
void destroy_user_locks(user_lock_array_t * locks);

int lock_trywait(const user_lock_array_t * locks, const char * username);
void lock_post(const user_lock_array_t * locks, const char * username);
void lock_close(const user_lock_array_t * locks, const char * username);

#endif