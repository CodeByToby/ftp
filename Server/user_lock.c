#include <stdio.h> // perror
#include <stdlib.h> // exit (exit types)
#include <unistd.h> // close
#include <string.h> // strncpy
#include <fcntl.h>  // (O_* constants)
#include <semaphore.h> // sem_*
#include <sys/wait.h> // wait

#include "../Common/string_helpers.h"
#include "user_lock.h"

#define CREDENTIALS_FILE "passwd"

static int generate_lockname(char * lockname, size_t lockname_size, const char * username) {
    return snprintf(lockname, lockname_size, "/ftp-%s", username);
}

/// @brief Create a set of locks for each user within credentials file. Run `destroy_user_locks` after you're done!
/// @param locks Where to save the locks.
/// @param cap The connection cap per user.
/// @return 0 if the locks have been successfully created, -1 for errors.
int create_user_locks(user_lock_array_t * locks, int cap) {
    FILE *fp;
    int nUsers = 0;
    char buffer[BUFFER_SIZE];

    if ((fp = fopen(CREDENTIALS_FILE, "r")) == NULL) {
        perror("fopen()");

        return -1;
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
            char lockname[BUFFER_SIZE];

            if(generate_lockname(lockname, sizeof(lockname), username) > sizeof(lockname)) {
                perror("snprintf(): Failed to create user locks:"); 
                free(locks->arr);
                
                return -1;
            }

            strncpy(locks->arr[i].username, username, BUFFER_SIZE);

            if((locks->arr[i].lock = sem_open(lockname, O_CREAT, 0600, cap)) == SEM_FAILED) {
                perror("sem_open(): Failed to create user locks:");
                free(locks->arr);
            
                return -1;
            }
        }

    fclose(fp);

    return 0;
}

/// @brief Destroy all of the user locks within `locks`.
/// @param locks A set of all locks to be destroyed.
void destroy_user_locks(user_lock_array_t * locks) {
    for (int i = 0; i < locks->size; ++i) {
        char lockname[BUFFER_SIZE];

        generate_lockname(lockname, sizeof(lockname), locks->arr[i].username);

        sem_close(locks->arr[i].lock);
        sem_unlink(lockname);
    }

    free(locks->arr);
}

/// @brief `sem_trywait` wrapper on a user-specific lock.
/// @param locks Set of all locks.
/// @param username Username for the lock.
/// @return `sem_trywait` return values, or -1 if lock has not been found.
int lock_trywait(const user_lock_array_t * locks, const char * username) {
    for(int i = 0; i < locks->size; ++i) {
        if(strncmp_size(locks->arr[i].username, username) == TRUE)
            return sem_trywait(locks->arr[i].lock);
    }

    return -1;
}

/// @brief `sem_post` wrapper on a user-specific lock.
/// @param locks Set of all locks.
/// @param username Username for the lock.
void lock_post(const user_lock_array_t * locks, const char * username) {
    for(int i = 0; i < locks->size; ++i) {
        if(strncmp_size(locks->arr[i].username, username) == TRUE) {
            sem_post(locks->arr[i].lock);
            return;
        }
    }

    return;
}

/// @brief `sem_close` wrapper on a user-specific lock.
/// @param locks Set of all locks.
/// @param username Username for the lock.
void lock_close(const user_lock_array_t * locks, const char * username) {
    for(int i = 0; i < locks->size; ++i) {
        if(strncmp_size(locks->arr[i].username, username) == TRUE) {
            sem_close(locks->arr[i].lock);
            return;
        }
    }
    
    return;
}