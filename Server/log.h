#ifndef LOG_H
#define LOG_H

void log_info(char * msg, int pid);
void log_erro(char * msg, int pid);
void log_comm(char * msg, int pid, command_t * cmd);
void log_resp(int pid, response_t * res);

#endif