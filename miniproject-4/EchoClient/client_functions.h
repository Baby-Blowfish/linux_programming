#ifndef FUNCTION_H
#define FUNCTION_H

#define BUF_SIZE 100
#define NAME_SIZE 20

void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_handling(char *msg);

extern char name[NAME_SIZE];
extern char msg[BUF_SIZE];

#endif
