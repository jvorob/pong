#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>

#include "util.h"

//PORT defined in util.h

int start_client();
void send_to_server(char* buff, int length, int sockfd);
void client_init_term();
void client_fix_term(int sig);

#endif
