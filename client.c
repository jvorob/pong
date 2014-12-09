
#include "client.h"

//PORT defined in util.h

#define OLD_IP "107.170.106.89"
#define LOCALHOST "127.0.0.1"

#define PORT_BUFF_SIZE 50


struct termios old_tio;

int start_client() {
	signal(SIGINT, client_fix_term);//If ctrl-c'ed, needs to return the terminal to normal

        printf("Starting client\n");
	client_init_term();

        struct addrinfo hints, *servinfo;
        hints.ai_family = PF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        memset(&hints, 0, sizeof(hints));


	char address[PORT_BUFF_SIZE];
	FILE *ip_file = fopen("ip.cfg", "r");
	if(ip_file == NULL) {
		warn("Failed to open ip.cfg");
		client_fix_term(0);
	}
	fgets(address, PORT_BUFF_SIZE, ip_file);
	fclose(ip_file);

	printf("Read address in from ip.cfg %s\n", address);

        // Open connection to server here
        int rv = getaddrinfo(address, PORT, &hints, &servinfo);
        if (rv != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		client_fix_term(0);
        }

        // loop through all the results and connect to the first we can
        struct addrinfo *p;
        int sockfd;
        for(p = servinfo; p != NULL; p = p->ai_next) {
                sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
                if (sockfd == -1) {
                        perror("client: socket");
                        continue;
                }

                if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                        close(sockfd);
                        perror("client: connect");
                        continue;
                }

                break;
        }

        if (p == NULL) {
                fprintf(stderr, "client: failed to connect\n");
                return 2;
        }

        char s[INET6_ADDRSTRLEN];
        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
                        s, sizeof s);
        printf("client: connecting to %s\n", s);

        freeaddrinfo(servinfo); // all done with this structure

        const int MAX_RECV_SIZE = 1024;
        char recv_buf[MAX_RECV_SIZE];
        int numbytes;

        const int BUF_SIZE = 1024;
        char *buf = malloc(BUF_SIZE);

        while(1) {
                // Receive from server
                if ((numbytes = recv(sockfd, recv_buf, MAX_RECV_SIZE-1, 0)) == -1) {
                        perror("recv");
			client_fix_term(0);
                }

                recv_buf[numbytes] = '\0';
                printf("client: received '%s'\n", recv_buf);

		printf("Enter chars\n");
		while(1) {
			int charbuff;
			char tempchar;
			// Send input to server
			charbuff = getchar();
			//send_to_server(buf, sockfd);
			if(charbuff != -1) {
				tempchar = charbuff;
				send_to_server(&tempchar, 1, sockfd);
			}
		}
        }

	client_fix_term(0);
        close(sockfd);
        return 0;
}

void client_init_term() {
	struct termios new_tio;

	tcgetattr(STDIN_FILENO,&old_tio);

	/* we want to keep the old setting to restore them a the end */
	new_tio=old_tio;

	/* disable canonical mode (buffered i/o) and local echo */
	new_tio.c_lflag &=(~ICANON & ~ECHO);
	new_tio.c_cc[VMIN] = 0;
	new_tio.c_cc[VTIME] = 0;

	/* set the new settings immediately */
	tcsetattr(STDIN_FILENO,TCSANOW,&new_tio);

	if(setvbuf(stdin, NULL, _IONBF, 0) == -1)
		err(-1, "Failed to setbuff"); 
}

void send_to_server(char *msg, int length, int sockfd) {
        printf("Sending the following message to the "
                        "server: %c\n", *msg);

	if(send(sockfd, msg, 1, 0))
		perror("send");
	
}

void client_fix_term(int sig) {//Fixes things and exits
	tcsetattr(STDIN_FILENO,TCSANOW,&old_tio);
	printf("[2J[H");
	fflush(stdout);

	exit(0);
}
