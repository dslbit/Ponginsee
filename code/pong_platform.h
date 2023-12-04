#ifndef PONG_PLATFORM_H
#define PONG_PLATFORM_H

#define FALSE 0
#define TRUE 1

#define INTERNAL static
#define LOCAL static
#define GLOBAL static

#define STRINGIFY(_s) #_s
#define TO_STRING(_x) STRINGIFY(_x)

#define CAST(_x) (_x)

#define ARRAY_COUNT(_x) (sizeof((_x)) / sizeof((_x[0]))) /* in number of elements */

#define MIN(_x, _y) ((_x) < (_y) ? (_x) : (_y))

/* TODO: Types and later make it precise using compiler checks, c version checks and 'stdint.h' */
typedef signed   int        B32;

typedef unsigned char       U8;
typedef unsigned short      U16;
typedef unsigned int        U32;
typedef unsigned long long  U64;

typedef signed   char       S8;
typedef signed   short      S16;
typedef signed   int        S32;
typedef signed   long long  S64;

typedef float               F32;
typedef double              F64;

typedef struct GameBackBuffer GameBackBuffer;
struct GameBackBuffer {
  S32 width;
  S32 height;
  S32 bytes_per_pixel;
  S32 stride;
  void *memory;
};

typedef struct GameButtonState GameButtonState;
struct GameButtonState {
  B32 pressed;
  B32 released;
};

typedef struct GameControllerInput GameControllerInput;
struct GameControllerInput {
  GameButtonState start;
  GameButtonState back; /* also used for menu (escape key) */
  GameButtonState up;
  GameButtonState down;
  GameButtonState left;
  GameButtonState right;
};

typedef struct GameInput GameInput;
struct GameInput {
  float dt;
  GameControllerInput player1;
};

INTERNAL void game_update_and_render(GameBackBuffer *back_buffer, GameInput *input);

#endif /* PONG_PLATFORM_H */
