#ifndef LOG_H
#define LOG_H

#include "../Common/packets.h"

void log_info(const char * msg, const int pid);
void log_erro(const char * msg, const int pid);
void log_comm(const char * msg, const int pid, const command_t * cmd);
void log_resp(const int pid, const response_t * res);

#endif