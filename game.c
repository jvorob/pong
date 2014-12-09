#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#define WIDTH 120
#define HEIGHT 50
#define PADDLE_HEIGHT 7
#define SPEED_INCREASE 1.05

#define START_VEL_X 0.5
#define START_VEL_Y 0.22

#define PADDLE_BOTTOM(paddle_top) ((paddle_top) + PADDLE_HEIGHT - 1)

#define TICK_TIME 30 * 1000 //uSeconds

int game_main(double *paddle_addr, int pid);

void init_term();
void init_game();
void update();
void draw_buff();
void print_buff();

void lose();
void restart();

void fix_term(int sig);//also exits

enum {
	PLAYING, LOST, QUIT
};

int state = PLAYING;
char buffer[HEIGHT][WIDTH + 1];

struct termios old_tio;

double *net_paddle;
int serv_pid;

double ball_x;
double ball_y;
double vel_x;
double vel_y;
int l_paddle;//coords of the top
int r_paddle;


int game_main(double *paddle_addr, int pid) {
	signal(SIGINT, fix_term);//If ctrl-c'ed, needs to return the terminal to normal

	//net_paddle has been shmget'ed
	//It's shmdt'ed in fix_term
	//is a double from 0 to 1
	net_paddle = paddle_addr; 
		
	//printf("Net-Paddle: %lf\n", *net_paddle);
	*net_paddle = 0.5;
	//printf("Net-Paddle: %lf\n", *net_paddle);

	serv_pid = pid;


	init_term();
	init_game();
	
	while(state != QUIT) {
		usleep(TICK_TIME);
		update();
		draw_buff();
		print_buff();
		printf("vel_x: %lf, vel_y %lf\n", vel_x, vel_y);
		//printf("Net-Paddle: %lf\n", *net_paddle);
	}

	printf("EXITING\n");
	fix_term(0);

	return 0;
}

void init_term() {
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

void init_game() {
	int i;

	vel_y = START_VEL_Y;
	vel_x = START_VEL_X;

	ball_y = 5;
	ball_x = 2;

	l_paddle = r_paddle = (HEIGHT - PADDLE_HEIGHT) / 2;

	memset(buffer, ' ', sizeof(buffer));
	for(i = 0; i < HEIGHT; i++) {
		buffer[i][WIDTH] = '\0'; //Null terminate them
		buffer[i][0] = '#'; //Make left and right walls
		buffer[i][WIDTH - 1] = '#';
	}

	for(i = 0; i < WIDTH; i++) { //Make top and bottom walls
		buffer[0][i] = '#';
		buffer[HEIGHT - 1][i] = '#';
	}

}

void update() {
	char c;
	while(read(STDIN_FILENO, &c, 1) != 0) {
		if(c == 'q') {
			state = QUIT;
		} else if(c == 'j') {
			r_paddle += 1;
			if(r_paddle + PADDLE_HEIGHT + 1 > HEIGHT)
				r_paddle = HEIGHT - PADDLE_HEIGHT - 1;
		} else if(c == 'k') {
			r_paddle -= 1;
			if(r_paddle < 1)
				r_paddle = 1;
		} else if(c == 'r' && state == LOST) {
			restart();
		}
	}

	//l_paddle = 1 + (int)((HEIGHT - 2 - PADDLE_HEIGHT) * (*net_paddle));

	switch(state) {
	case PLAYING:
		ball_x += vel_x;
		ball_y += vel_y;


		//for collisions: if ball_x >= n and  < n + 1, then the ball is in tile n

		if(ball_y >= HEIGHT - 1 || ball_y < 1) { //bounce off top and bottom

			vel_y *= -1;
			ball_y += vel_y;
		}

		if(ball_x >= WIDTH - 2 || ball_x < 2) {//check paddles
			int temp_paddle = ball_x < 2 ? l_paddle : r_paddle;
			int hitdist = (int) ball_y - temp_paddle - PADDLE_HEIGHT / 2;

			if(ball_y >= temp_paddle && ball_y < temp_paddle + PADDLE_HEIGHT) {//If theres a paddle there
				vel_x *= -1 * SPEED_INCREASE;
				ball_x += vel_x;
				vel_y += (double)hitdist / 50;
			} else {
				lose();
			}
		}

		l_paddle = ball_y - PADDLE_HEIGHT / 2;

		break;
	case LOST:
		break;
	case QUIT:
		printf("Should be quitting\n");
		break;
	default:
		err(-1, "Invalid state");
	}
	

}

void draw_buff() {
	int i, j;

	if(state == PLAYING) {
		//clear the interior
		for(i = 1; i < WIDTH - 1; i++)
			for(j = 1; j < HEIGHT - 1; j++)
				buffer[j][i] = ' ';

		buffer[(int) ball_y][(int) ball_x] = '*';

		for(i = 0; i < PADDLE_HEIGHT; i++) {
			buffer[l_paddle + i][1] = '|';
			buffer[r_paddle + i][WIDTH - 2] = '|';
		}
	}
}

void print_buff(){
	int i;

	printf("[H");

	for(i = 0; i < HEIGHT; i++)
		printf("%s\n", buffer[i]);
}

void lose(){
	char gameover_string[] = "Game Over: r restart, q quit";

	draw_buff();

	state = LOST;

	if(WIDTH < 50)
		err(-1, "Screen too small");

	strcpy(buffer[HEIGHT / 2] + (WIDTH - sizeof(gameover_string)) / 2, gameover_string);
}

void restart() {
	init_game();
	state = PLAYING;
}

void fix_term(int sig) {
	int status;

	tcsetattr(STDIN_FILENO,TCSANOW,&old_tio);
	printf("[2J[H");
	fflush(stdout);

	shmdt(net_paddle);
	printf("Did shmdt\n");
	printf("Killing server: pid %d\n", serv_pid);
	kill(serv_pid, SIGINT);
	printf("Killed server\n");
	wait(&status);
	printf("Reaped child\n");

	exit(0);
}
