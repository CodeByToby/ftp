#ifndef COMMAND_TYPES_H
#define COMMAND_TYPES_H

enum COMMAND_TYPE {
    LIST,   /* List files and directories */
    HELP,   /* Print information about usage */

    RETR,   /* Retrieve file from server */
    STOR,   /* Upload file to server */
    STOU,   /* Upload uniquely-named file to server */
    APPE,   /* Append or create file on server */
    DELE,   /* Delete file on server */

    RMD,    /* Remove directory on server */
    MKD,    /* Make new directory on server */
    PWD,    /* Print working directory */
    CWD,    /* Change working directory */
    CDUP,   /* Change to parent directory */

    PASS,   /* Provide server with password */
    USER,   /* Provide server with username */

    NOOP,   /* No operation */
    QUIT    /* Terminate the connection for this client */
};

#endif