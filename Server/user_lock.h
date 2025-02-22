#ifndef USER_LOCK_H
#define USER_LOCK_H

#include <semaphore.h>
#include <sys/wait.h> // wait
#include <semaphore.h> // sem_*
#include <fcntl.h>  // O_* constants
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../Common/packets.h"
#include "../Common/defines.h"

#define CREDENTIALS_FILE "passwd"

typedef struct user_lock {
    char username[BUFFER_SIZE];
    char lockname[BUFFER_SIZE];
    sem_t * lock;
} user_lock_t;

typedef struct user_lock_array {
    user_lock_t * arr;
    int size;
} user_lock_array_t;


void create_user_locks(user_lock_array_t * locks, int sockfd, int cap)
{
    FILE *fp;
    int nUsers = 0;
    char buffer[BUFFER_SIZE];
    char semName[BUFFER_SIZE];

    if ((fp = fopen(CREDENTIALS_FILE, "r")) == NULL) {
        perror("fopen()");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

        // Count all users
        while (fgets(buffer, BUFFER_SIZE, fp) != NULL)
            nUsers++;

        fseek(fp, 0, SEEK_SET);

        locks->size = nUsers;
        locks->arr = (user_lock_t*) malloc(nUsers * sizeof(user_lock_t));

        // Create user locks
        for(int i = 0; fgets(buffer, BUFFER_SIZE, fp) != NULL; ++i) {
            char * username = strtok(buffer, ":");

            if(snprintf(semName, sizeof(semName), "/ftp-%s", username) > sizeof(semName)) {
                perror("snprintf(): Failed to create user locks:"); 
                close(sockfd);
                free(locks->arr);
                exit(EXIT_FAILURE);
            }

            strncpy(locks->arr[i].username, username, BUFFER_SIZE);
            strncpy(locks->arr[i].lockname, semName, BUFFER_SIZE);

            if((locks->arr[i].lock = sem_open(semName, O_CREAT | O_EXCL, 0600, cap)) == SEM_FAILED) {
                perror("sem_open(): Failed to create user locks:");
                close(sockfd);
                free(locks->arr);
                exit(EXIT_FAILURE);
            }        
        }

    fclose(fp);
}

void destroy_user_locks(user_lock_array_t * locks) {
    for (int i = 0; i < locks->size; ++i) {
        sem_close(locks->arr[i].lock);
        sem_unlink(locks->arr[i].lockname);
    }

    free(locks->arr);
}


#endif