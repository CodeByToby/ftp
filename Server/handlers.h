#ifndef HANDLERS_H
#define HANDLERS_H

#include "../Common/packets.h"
#include "../Common/defines.h"

int ftp_list(int sockfd_accpt, response_t * res);
//int ftp_retr(int sockfd_accpt, response_t * res);
//int ftp_stor(int sockfd_accpt, response_t * res);
//int ftp_stou(int sockfd_accpt, response_t * res);
//int ftp_appe(int sockfd_accpt, response_t * res);
//int ftp_dele(int sockfd_accpt, response_t * res);
//int ftp_rmd(int sockfd_accpt, response_t * res);
//int ftp_mkd(int sockfd_accpt, response_t * res);
//int ftp_pwd(int sockfd_accpt, response_t * res);
//int ftp_cwd(int sockfd_accpt, response_t * res);
//int ftp_cdup(int sockfd_accpt, response_t * res);
int ftp_noop(int sockfd_accpt, response_t * res);
//int ftp_quit(int sockfd_accpt, response_t * res);
//int ftp_help(int sockfd_accpt, response_t * res);

#endif