#define PORT "48879"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

#define SHMGET_KEY 24601

#define MAX_SOCKS 256


//488879 is 0xBEEF

#include "server.h"

double *net_paddle;
int shmem_d;

int sockets[MAX_SOCKS];
int last_num[MAX_SOCKS];
int num_socks;

int accept_connections = 1;//gets set to 0 by SIGUSR1

//Will eventually prepare many socket descriptors and put them in sockets_out
int start_server(int sockets_out[], int max_sockets){
	signal(SIGINT, cleanup);
	signal(SIGUSR1, stop_accepting);

        printf("Server starting\n");

	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	int i;

	//BEGIN PREPARING SOCKETS
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) { //prepares servinfo with own ip on port PORT
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
	//SOCKETS PREPARED

	printf("server: waiting for connections...\n");

	for(i = 0; i < MAX_SOCKS; i++) {
		sockets[i] = -1;
		num_socks = 0;
	}

	//I don't want to handle connections intelligently, so imma do this instead
	int pid;
	fcntl(sockfd, F_SETFL, O_NONBLOCK);
	if(pid = fork()) {//Parent
		printf("I'm parent: %d\n", getpid());
		printf("Child: %d\n", pid);
		while(accept_connections) {  // main accept() loop
			sin_size = sizeof their_addr;
			new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
			if (new_fd == -1) {
				if(errno == EAGAIN || errno == EWOULDBLOCK)//if no connections
					continue;
				perror("accept");
				continue;
			}

			fcntl(new_fd, F_SETFL, O_NONBLOCK);

			inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *)&their_addr),
				s, sizeof s);
			printf("server: got connection from %s\n", s);

			sockets[num_socks] = new_fd;
			num_socks++;

		}
	} else {
		printf("Parent is %d\n", getppid());
		printf("Accepting connections: press any key to finish\n");
		getchar();//Should wait for input
		printf("Got one, ending server listening\n");
		kill(getppid(), SIGUSR1);
		exit(0);
	}

	waitpid(pid, NULL, 0);
	server_body(sockets);

        return 0;
}


void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

double *prepare_memory(int flags) { //shmget's a double, shmat's it, and returns it 
	double *temp;

	if((shmem_d = shmget(SHMGET_KEY, sizeof(double), 0644 | flags)) == -1)
		err(-1, "Failed to shmget()\n");

	if((long)(temp = shmat(shmem_d, NULL, 0)) == -1)
		err(-1, "Failed to shmat()\n");


	return temp;
}


void cleanup(int sigid) {//cleans up and exits
	printf("Cleaning up memory\n");
	shmctl(shmem_d, IPC_RMID, NULL);
	shmdt(net_paddle);
	exit(0);
}

void stop_accepting(int sigid) {//sets accept_connections to 0
	accept_connections = 0;
}

void server_body() {//Takes a socket descriptor, gets shit done
	int pid;
	int i;

	pid = fork();

	if(pid) { //Parent
		net_paddle = prepare_memory(0666 | IPC_CREAT);
		for(i = 0; i < num_socks; i++) {
			if(sockets[i] != -1)
				close(sockets[i]);
		}
		game_main(net_paddle, pid);
	} else { //Child
		net_paddle = prepare_memory(0666 | IPC_CREAT);
		char msg[] = "You have connected\n";
		char temp;
		int count, total;
		for(i = 0; i < num_socks; i++) {//Send handshakes to all
			if(sockets[i] != -1);
				send(sockets[i], msg, sizeof(msg), 0);
			last_num[i] = -1; //Clear last_num
		}
		printf("Sent handshakes\n");
		while(1) {
			count = total = 0;
			for(i = 0; i < num_socks; i++) {
				if(recv(sockets[i], &temp, 1, 0) > 0) {
					printf("Received %c from %d\n", temp, i);
					if(temp == 'j' || temp == 's') {
						last_num[i] = 1;
					} else if(temp == 'k' || temp == 'w') {
						last_num[i] = 0;
					}
				}
				if(last_num[i] != -1) {
					total += last_num[i];
					count ++;
				}
			}
			if(count > 0) {
				*net_paddle = (double)total / (double)count;
			}
		}
	}
}
