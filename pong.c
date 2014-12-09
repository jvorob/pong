#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "util.h"

#include "client.h"
#include "server.h"

#define BUF_SIZE 256



int main(int argc, char *argv[]) {

        if(argc != 2)
                errx(0, "Usage: pong [--client|--server]\n");

        if(!strcmp(argv[1], "--client")) {
                return start_client();
        } else if(!strcmp(argv[1], "--server")) {
                return start_server();
        } else {
                errx(0, "Usage: pong [--client|--server]\n");
        }
}
