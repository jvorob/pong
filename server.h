#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <fcntl.h>

#include "util.h"

//PORT defined in util.h

int start_server();
void sigchld_handler(int s);

double *prepare_memory(int flags); //shmget's a double, shmat's it, and returns it 
void cleanup(int sigid);//cleans up and exits
void stop_accepting(int sigid);//sets accept_connections to 0

void server_body();//Takes a socket descriptor, gets shit done
#endif
