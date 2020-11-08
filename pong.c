#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <ncurses.h>

#define MAX_X_VELOCITY 2
#define MAX_Y_VELOCITY 2
#define WIDTH 100
#define HEIGHT 40
#define PADDLE_WIDTH 2
#define PADDLE_HEIGHT 8
#define BALL_SIZE 2
#define MOVEMENT_SPEED 3
#define PADDLE_CHAR 'X'
#define BALL_CHAR 'O'
#define BLANK_CHAR '.'
#define INPUT_BUFFER_SIZE 24
#define REFRESH_SPEED 50000

struct inputBufferStruct {
  char * inputBuffer;
  int  inputBufferPointer;
};

struct position {
  int x, y;
};

void * getAndHandleInput ( void * input ) {
  struct inputBufferStruct * inputBuffer =
    (struct inputBufferStruct * ) input;

  while (1) {
    char buffer = (char) getch();
    if (inputBuffer->inputBufferPointer < INPUT_BUFFER_SIZE) {
      inputBuffer->inputBuffer[inputBuffer->inputBufferPointer] = buffer;
      inputBuffer->inputBufferPointer++;
    }
  }

  pthread_exit(NULL);
}

void createInputReadingThread ( struct inputBufferStruct * bufferStruct ) {
  // Create input reading thread

  pthread_t inputThread;
  int inputThreadStatus = pthread_create(&inputThread, NULL,
    getAndHandleInput, (void *) bufferStruct);

  if (inputThreadStatus) {
    printw("Error: Unable to create input reading thread, %d\n",
      inputThreadStatus);
    exit(-1);
  }
}

void createAndPrintMatrix( int playerPaddleY, int aiPaddleY,
  struct position ball, int score ) {
  clear();

  printw("Score: %i\n", score);
  char c;

  for (int y = 0; y < HEIGHT; y++) {
    for (int x = 0; x < WIDTH; x++) {
      if (x < PADDLE_WIDTH
        && y <= playerPaddleY + PADDLE_HEIGHT && y >= playerPaddleY) {
        c = PADDLE_CHAR;
      } else if (x >= WIDTH - PADDLE_WIDTH
              && y <= aiPaddleY + PADDLE_HEIGHT && y >= aiPaddleY ) {
        c = PADDLE_CHAR;
      } else if (x >= ball.x && x < ball.x + BALL_SIZE
              && y >= ball.y && y < ball.y + BALL_SIZE) {
        c = BALL_CHAR;
      } else {
        c = BLANK_CHAR;
      }
      printw("%c", c);
    }
    printw("\n");
  }
  refresh();
}

int moveBall(int * xVelocity, int * yVelocity, struct position * ball,
  int * playerPaddleY, int * aiPaddleY, int * score ) {
  ball->x += *xVelocity;
  ball->y += *yVelocity;

  //Bounce off walls
  if (ball->x + BALL_SIZE > WIDTH - PADDLE_WIDTH) {
    ball->x = WIDTH - BALL_SIZE - PADDLE_WIDTH;
    *xVelocity *= -1;
  } else if (ball->x < 0) {
    ball->x = 0;
    *xVelocity *= -1;
  }

  if(ball->y + BALL_SIZE > HEIGHT) {
    ball->y = HEIGHT - BALL_SIZE;
    *yVelocity *= -1;
  } else if (ball->y < 0) {
    ball->y = 0;
    *yVelocity *= -1;
  }

  //Bounce of paddles
  if (ball->x <= PADDLE_WIDTH) {
    *xVelocity *= -1;
    *yVelocity *= -1;

    if (ball->y < *playerPaddleY
      || ball->y > *playerPaddleY + PADDLE_HEIGHT) {
      return 1;
    } else {
      *score += 1;
      if(ball->y >= *playerPaddleY + PADDLE_HEIGHT / 4
        && ball->y < *playerPaddleY + 3 * PADDLE_HEIGHT / 4) {
          *xVelocity += 1;
      } else if (*yVelocity > 0) {
          *yVelocity += 1;
      } else {
          *yVelocity -= 1;
      }
    }
  }

  if (ball->x >= WIDTH - PADDLE_WIDTH) {
    *xVelocity *= -1;
    *yVelocity *= -1;
  }

  return 0;
}

void movePaddles( struct inputBufferStruct * inputBuffer,
  int * playerPaddleY, int * aiPaddleY, struct position * ball ) {
  while (inputBuffer->inputBufferPointer > 0) {

    inputBuffer->inputBufferPointer--;
    char input = inputBuffer->inputBuffer[inputBuffer->inputBufferPointer];

    if ( (input == 'w' || input == 'W')
      && *playerPaddleY > 0) {
      *playerPaddleY -= MOVEMENT_SPEED;

      if (*playerPaddleY < 0)
        *playerPaddleY = 0;
    } else if ((input == 's' || input == 'S')
        && *playerPaddleY < HEIGHT - PADDLE_HEIGHT) {
      *playerPaddleY += MOVEMENT_SPEED;

      if (*playerPaddleY > HEIGHT - PADDLE_HEIGHT)
        *playerPaddleY = HEIGHT - PADDLE_HEIGHT;
    }
  }

  *aiPaddleY = ball->y - BALL_SIZE / 2 - PADDLE_HEIGHT / 2;
}

void gameLoop ( struct inputBufferStruct * inputBuffer ) {
  int lost = 0, score = 0, playerPaddleY = 0, aiPaddleY = 0;
  int xVelocity = 1;
  int yVelocity = 1;

  struct position ball = {WIDTH / 2, HEIGHT / 2};

  while (lost == 0) {
    movePaddles(inputBuffer, &playerPaddleY, &aiPaddleY, &ball);
    lost = moveBall(&xVelocity, &yVelocity, &ball,
      &playerPaddleY, &aiPaddleY, &score);

    createAndPrintMatrix(playerPaddleY, aiPaddleY, ball, score);

    usleep(REFRESH_SPEED);
  }
}

int main () {
  char * inputBuffer = malloc(sizeof(char) * INPUT_BUFFER_SIZE);

  if (inputBuffer == 0) {
    exit(1);
  }

  struct inputBufferStruct bufferStruct = {inputBuffer, 0};

  initscr();
  cbreak();
  noecho();

  createInputReadingThread(&bufferStruct);
  gameLoop(&bufferStruct);
  free(inputBuffer);

  printw("\n");
  endwin();
  exit(0);
}
