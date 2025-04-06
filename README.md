# FTP client/server implementation

A server/client FTP implementation in C.

## Server

The server implementation recognises these commands:
- **USER**, **PASS**
- **NOOP**, **QUIT**, **HELP**
- **PASV**, **PORT** (only as a stub)
- **PWD**, **MKD**, **RMD**, **CWD**
- **LIST**, **RETR**, **STOR**

### Credentials

All credentials are stored within the '_passwd_' file stored in the _build_ directory of the server. The file **must** exist in the same folder as the compiled binary.

The file contains lines of the form: `user_name:password:root_folder`; each line represents a single user.

### Client handling

On establishing a connection, a new process is forked, which handles the user commands.

The client instance is isolated in it's own root, within the specified directory in credentials file. 
Only one client can be logged in as a specified user, other clients will be unable to log in with these credentials. 
This is enforced with _user locks_, specified in `user_locks.c`.

### FTP standard

The server utilizes two types of ports – the **control port** and **the data port** – to establish a connection with the client, as per the standard defined in RFC 959. 
Commands, and most return values of these commands are also compliant with the standard.

One thing to mention is that the packets sent between the client and the server are structs, and not c-strings, which is nonstandard behavior.

## Client

The client implementation handles user input, as well as sending commands and receiving messages. The implementation is frankly shoddy, but it works.

## Compiling

Both of the programs are compiled by running ``` make ``` within the program's folder. You can also run ``` make all ``` from one folder up to compile both programs at the same time.
