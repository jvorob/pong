#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT "48879"  // the port users will be connecting to
//488879 is 0xBEEF

int _getline(char *in, int bytestoread);
void print_prompt();
void print_help();
void *get_in_addr(struct sockaddr*);

#endif
