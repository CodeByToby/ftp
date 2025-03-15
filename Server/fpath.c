#include <stdio.h> // printf
#include <stdlib.h> // realpath NULL
#include <unistd.h> // getcwd
#include <string.h> // strncpy memset strlen
#include <sys/stat.h> // stat

#include "../Common/packets.h"
#include "../Common/defines.h"
#include "commands.h"

int __update_fpath(const command_t * cmd, response_t * res, const user_session_t * session, char * resolvedPath, size_t path_len) {
    char newPath[path_len];
    char oldPath[path_len];

    memset(newPath, 0, path_len);
    memset(oldPath, 0, path_len);

    if(cmd->args[0] == '/') {
        // Parse as absolute path
        if(snprintf(newPath, path_len, "%s/%s", session->root, cmd->args) > path_len) {
            response_set(res, 550, "Requested action not taken. Argument too long");
            return -1;
        }
    } else {
        // Parse as relative path
        getcwd(oldPath, sizeof(oldPath));

        if(snprintf(newPath, path_len, "%s/%s", oldPath, cmd->args) > path_len) {
            response_set(res, 550, "Requested action not taken. Argument too long");
            return -1;
        }
    }

    // Resolve path, get rid of . and .. and additional slashes
    if(realpath(newPath, resolvedPath) == NULL) {
        response_set(res, 550, "Requested action not taken. Path is invalid or does not exist");
        return -1;
    }

    // Check if path is outside of root
    if(strncmp(session->root, resolvedPath, strlen(session->root)) != 0) {
        response_set(res, 550, "Requested action not taken. Path out of scope");
        return -1;
    }

    return 0;
}