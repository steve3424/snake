/*
 *  Most of the code comes from sergeyrar who posted a question on stack exchange:
 *  'codereview.stackexchange.com/questions/132604/snake-game-in-c-for-linux-console'
 *  
 *  1. Tail is only used in snake_place
 *  2. Tail is modified in snake place to not point to the tail
 *  3. The tail position is subsequently accessed as the first element of the snake_pos array
 *  4. Since x,y are always accessed in pairs, it might make sense to store them as SoA instead of AoS
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <ncurses.h>

#define _height 50
#define _width 100
#define _padding 5

#define _up 119
#define _down 115
#define _left 97
#define _right 100

#define _start 5
#define _snake_size 3



typedef struct Pose {
	int x;
	int y;
} Pose;

typedef struct Snake {
	char symbol;
	int size;
	char direction;
	char prev_direction;
	// body contains an x,y Pose for each segment of the body
	// the point of this is to be able to check if the snake collides with itself
	// tail is at index 0
	// head is at index size - 1
	// array is size of whole board to allow for growth of snake when eating food
	Pose body[_height*_width*sizeof(Pose)];
} Snake;

typedef struct Food {
	int x;
	int y;
	char symbol;
} Food;



void snake_init(Snake *snake1);
void food_init(Food *food1);
void gotoxy(int,int);
void draw_borders();
void draw_food(Food *food1);
void draw_snake(Snake *snake1);
int game_over(Snake *snake1);
void move_snake(Snake *snake1, Food *food1, int *score);
void move_head(Snake *snake1);
void move_tail(Snake *snake1);
int kbhit(void);


int main() {
	useconds_t speed = 100000;
      	int score = 0;

	// Initialize entities 
	srand(time(0));
      	Snake snake1;
      	Food food1;
      	snake_init(&snake1);
	food_init(&food1);	

	// Draw game
      	printf("\e[1;1H\e[2J");
	system("stty -echo");
	draw_borders();
	draw_food(&food1);
	draw_snake(&snake1);

	system ("/bin/stty raw");

	// Game loop
      	while(!game_over(&snake1)) {
          	while (!kbhit())
          	{
			usleep(speed);
			move_snake(&snake1, &food1, &score);		
			if (game_over(&snake1)) {
                     		break;
                 	}
          	}
		snake1.prev_direction = snake1.direction;
		snake1.direction = getchar();
     	}

	system("/bin/stty cooked");
    	system("stty echo");
	gotoxy(0, _height + 1);
	printf("\n\n Final score: %d \n\n", score);

     	return 0;
}




void snake_init(Snake *snake1) {
	snake1->symbol = '*';
	snake1->size = _snake_size;
	snake1->direction = _right;
	snake1->prev_direction = _right;

	// init snake starting position
	// tail starts at (_start, _start) and points right
	memset(snake1->body, 0, sizeof(Pose));
	for (int i=0; i < snake1->size; ++i) {
		Pose pos = {_start + i, _start};
		snake1->body[i] = pos;
	}	
}

void food_init(Food *food1) {
	food1->x = rand() % (_width - _padding*2) + _padding; 
	food1->y = rand() % (_height - _padding*2) + _padding;
	food1->symbol = 'F';
}

void draw_borders() {
	int i;
	for (i=0; i < _height; ++i) {
		gotoxy(0, i);
		printf("X");
		gotoxy(_width, i);
		printf("X");
	}

	for (i=0; i < _width; ++i) {
		gotoxy(i, 0);
		printf("X");
		gotoxy(i, _height);
		printf("X");
	}
}

void draw_food(Food *food1) {
	gotoxy(food1->x, food1->y);
	printf("%c", food1->symbol);
}

void draw_snake(Snake *snake1) {
	for (int i=0; i < snake1->size; ++i) {
		Pose pos = snake1->body[i];
		gotoxy(pos.x, pos.y);
		printf("%c", snake1->symbol);
	}
}

// moves cursor to location for printing
void gotoxy(int x,int y) {
		printf("%c[%d;%df",0x1B,y,x);
}

int game_over(Snake *snake1) {
	int head_index = snake1->size - 1;
	Pose head = snake1->body[head_index];

	// crashed into body
	for (int i=0; i < head_index; ++i) {
		if (snake1->body[i].x == head.x && snake1->body[i].y == head.y) {
			return 1;
		}
	}

	// crashed into wall
	if (head.x == _width || head.x == 1 || head.y == _height || head.y == 1) {
		return 1;
	}

	return 0;
}

void move_snake(Snake *snake1, Food *food1, int *score) {
	move_head(snake1);

	Pose head = snake1->body[snake1->size - 1];
	bool ate_food = (head.x == food1->x) && (head.y == food1->y); 
	if (!ate_food) {
		move_tail(snake1);
	}
	else {
		snake1->size++;
		(*score)++;
		food_init(food1);
		draw_food(food1);
	}
}

void move_head(Snake *snake1) {
	// create new head based on direction change
	Pose new_head = snake1->body[snake1->size - 1];
	switch (snake1->direction) {
		case _right:
			if (snake1->prev_direction == _left) {
				new_head.x--;
				break;
			}
			new_head.x++;
			break;
		case _left:
			if (snake1->prev_direction == _right) {
				new_head.x++;
				break;
			}
			new_head.x--;
			break;
		case _up:
			if (snake1->prev_direction == _down) {
				new_head.y++;
				break;
			}
			new_head.y--;
			break;
		case _down:
			if (snake1->prev_direction == _up) {
				new_head.y--;
				break;
			}
			new_head.y++;
			break;
		default:
			break;
	}

	// insert new head into body array and draw 
	snake1->body[snake1->size] = new_head;
	gotoxy(new_head.x, new_head.y);
	printf("%c", snake1->symbol);
}

void move_tail(Snake *snake1) {
	// remove end of tail
	Pose tail = snake1->body[0];
	gotoxy(tail.x, tail.y);
	printf(" ");

	// update body positions 
	for (int i=0; i < snake1->size; ++i) {
		snake1->body[i] = snake1->body[i+1];	
	}
}

int kbhit(void) {
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if(ch != EOF) {
		ungetc(ch, stdin);
		return 1;
	}

	return 0;
}



