#ifndef HANDLERS_H
#define HANDLERS_H

#include "../Common/packets.h"
#include "../Common/defines.h"

int ftp_list(response_t * res);
int ftp_retr(response_t * res);
int ftp_stor(response_t * res);
int ftp_stou(response_t * res);
int ftp_appe(response_t * res);
int ftp_dele(response_t * res);
int ftp_rmd(response_t * res);
int ftp_mkd(response_t * res);
int ftp_pwd(response_t * res);
int ftp_cwd(response_t * res);
int ftp_cdup(response_t * res);
int ftp_noop(response_t * res);
int ftp_quit(response_t * res);
int ftp_help(response_t * res);

#endif