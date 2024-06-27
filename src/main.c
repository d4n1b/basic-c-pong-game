#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include "./constants.h"
#include "./game.h"

void setup();
void restart();
void update(float delta_time);
void render();
void process_input();
int init_window();
void destroy_window();
bool detect_collision(GameObject *a, GameObject *b);
void handle_collision(GameObject *ball, GameObject *paddle);

bool debug = false;
bool quit = false;
bool game_over = false;
bool paused = false;
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

GameObject ball = {};
GameObject paddle = {};

float speed = SPEED_INITIAL;

int main(int argc, char *argv[])
{
  // Check arguments
  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "--debug") == 0)
      debug = true;
  }

  setup();
  restart();

  Uint32 last_frame_time = SDL_GetTicks();

  while (!quit)
  {
    Uint32 current_time = SDL_GetTicks();
    float delta_time = (current_time - last_frame_time) / 1000.0f;

    process_input();
    update(delta_time);
    render();

    last_frame_time = SDL_GetTicks();

    // Cap the frame rate
    Uint32 frame_duration = SDL_GetTicks() - current_time;
    if (frame_duration < FRAME_TIME)
    {
      SDL_Delay(FRAME_TIME - frame_duration);
    }
  }

  destroy_window();

  return 0;
}

void setup()
{
  if (!init_window())
  {
    quit = true;
  }
}

void process_input()
{
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {

    switch (event.type)
    {
    case SDL_QUIT:
      quit = true;
      break;

    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_ESCAPE)
        quit = true;

      // Capture paddle movement
      if (event.key.keysym.sym == SDLK_RIGHT)
        paddle.dx = 1.0;
      if (event.key.keysym.sym == SDLK_LEFT)
        paddle.dx = -1.0;
      break;

    case SDL_KEYUP:
      if (event.key.keysym.sym == SDLK_r)
        restart();

      // Stop paddle movement
      if (event.key.keysym.sym == SDLK_RIGHT)
        paddle.dx = 0.0;
      if (event.key.keysym.sym == SDLK_LEFT)
        paddle.dx = 0.0;

      // Toggle pause
      if (event.key.keysym.sym == SDLK_SPACE)
      {
        paused = !paused;

        if (game_over)
          restart();
      }
      break;
    }
  }
}

void update(float delta_time)
{
  if (paused)
    return;

  ball.x += ball.dx * speed * delta_time;
  ball.y += ball.dy * speed * delta_time;

  paddle.x += paddle.dx * speed * delta_time;

  // Check game over state
  if (ball.y + ball.height >= WINDOW_HEIGHT)
  {
    if (debug)
      printf("GameState: Game over\n");

    ball.y = WINDOW_HEIGHT - ball.height;
    paused = true;
    game_over = true;
    return;
  }

  // Ensure paddle is inside window's boundaries
  if (paddle.x <= 0)
  {
    paddle.x = 0;
  }
  if (paddle.x + paddle.width >= WINDOW_WIDTH)
  {
    paddle.x = WINDOW_WIDTH - paddle.width;
  }

  // Ensure ball is inside window's boundaries
  if (ball.x <= 0)
  {
    ball.x = 0;
    ball.dx *= -1;
    speed *= SPEED_INCREASE_FACTOR;
  }
  if (ball.x + ball.width >= WINDOW_WIDTH)
  {
    ball.x = WINDOW_WIDTH - ball.width;
    ball.dx *= -1;
    speed *= SPEED_INCREASE_FACTOR;
  }
  if (ball.y <= 0)
  {
    ball.y = 0;
    ball.dy *= -1;
    speed *= SPEED_INCREASE_FACTOR;
  }

  // Check ball collision against paddle
  handle_collision(&ball, &paddle);
}

void render()
{
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  // Render elements
  SDL_Rect ball_rect = {
      ball.x,
      ball.y,
      ball.width,
      ball.height,
  };
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderFillRect(renderer, &ball_rect);

  SDL_Rect paddle_rect = {
      paddle.x,
      paddle.y,
      paddle.width,
      paddle.height,
  };
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderFillRect(renderer, &paddle_rect);

  SDL_RenderPresent(renderer);
}

void restart()
{
  if (debug)
    printf("GameState: Game started\n");

  game_over = false;
  paused = false;
  speed = SPEED_INITIAL;

  ball.x = RANDOM(0, WINDOW_WIDTH);
  ball.y = RANDOM(0, WINDOW_HEIGHT / 3);
  ball.dx = 1.0;
  ball.dy = 1.0;
  ball.width = BALL_WIDTH;
  ball.height = BALL_HEIGHT;

  paddle.x = WINDOW_WIDTH / 2 - PADDLE_WIDTH / 2;
  paddle.y = WINDOW_HEIGHT - WINDOW_PADDING - PADDLE_HEIGHT;
  paddle.dx = 0.0;
  paddle.dy = 0.0;
  paddle.width = PADDLE_WIDTH;
  paddle.height = PADDLE_HEIGHT;
}

int init_window()
{
  if (debug)
    printf("Window:create\n");
  if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
  {
    if (debug)
      printf("Error initialising SDL. SDL_Error: %s\n", SDL_GetError());
    return false;
  }

  window = SDL_CreateWindow(
      "SDL Tutorial",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      WINDOW_WIDTH,
      WINDOW_HEIGHT,
      SDL_WINDOW_INPUT_FOCUS);
  if (window == NULL)
  {
    if (debug)
      printf("Error creating SDL Window. SDL_Error: %s\n", SDL_GetError());
    return false;
  }

  renderer = SDL_CreateRenderer(window, -1, 0);
  if (renderer == NULL)
  {
    if (debug)
      printf("Error creating SDL Renderer. SDL_Error: %s\n", SDL_GetError());
    return false;
  }

  return true;
}

void destroy_window()
{
  if (debug)
    printf("Window:destroy\n");
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

// Axis-Aligned Bounding Box (AABB) Collision Resolution: https://www.youtube.com/watch?v=oOEnWQZIePs
bool detect_collision(GameObject *a, GameObject *b)
{
  return (
      a->x < b->x + b->width &&
      a->x + a->width > b->x &&
      a->y < b->y + b->height &&
      a->y + a->height > b->y);
}

void handle_collision(GameObject *ball, GameObject *paddle)
{
  if (detect_collision(ball, paddle))
  {
    if (debug)
      printf("GameState: collision detected\n");

    // Determine the penetration depth for each axis
    float ball_center_x = ball->x + ball->width / 2.0;
    float ball_center_y = ball->y + ball->height / 2.0;
    float paddle_center_x = paddle->x + paddle->width / 2.0;
    float paddle_center_y = paddle->y + paddle->height / 2.0;

    float dx = ball_center_x - paddle_center_x;
    float dy = ball_center_y - paddle_center_y;

    float combined_half_widths = (ball->width + paddle->width) / 2.0;
    float combined_half_heights = (ball->height + paddle->height) / 2.0;

    float overlap_x = combined_half_widths - fabs(dx);
    float overlap_y = combined_half_heights - fabs(dy);

    // Resolve the collision by determining the direction of penetration,
    // helping decide whether the collision is more significant on the X-axis or the Y-axis.
    if (overlap_x < overlap_y)
    {
      // Ball hits the sides of the paddle
      ball->dx *= -1;
      // Adjust ball position to avoid sticking
      if (dx > 0)
      {
        ball->x = paddle->x + paddle->width;
      }
      else
      {
        ball->x = paddle->x - ball->width;
      }
    }
    else
    {
      // Ball hits the top or bottom of the paddle
      ball->dy *= -1;
      // Adjust ball position to avoid sticking
      if (dy > 0)
      {
        ball->y = paddle->y + paddle->height;
      }
      else
      {
        ball->y = paddle->y - ball->height;
      }
    }
  }
}
