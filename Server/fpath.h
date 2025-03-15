#ifndef FPATH_H
#define FPATH_H

#include "../Common/packets.h"
#include "commands.h"

// Working file / directory path
char fpath[BUFFER_SIZE];

/// @brief Update `fpath` – working file / directory path. Uses data from `cmd.args` and `session.root`
/// @return 0 on success, -1 on errors.
#define update_fpath() __update_fpath(cmd, res, session, fpath, sizeof(fpath))

int __update_fpath(const command_t * cmd, response_t * res, const user_session_t * session, char * resolvedPath, size_t path_len);

#endif