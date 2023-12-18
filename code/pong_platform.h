#ifndef PONG_PLATFORM_H
#define PONG_PLATFORM_H

EXTERN_OPEN /* extern "C" { */

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
  union {
    GameButtonState buttons[6];
    struct {
      GameButtonState start;
      GameButtonState back; /* also used for menu (escape key / select on controller) */
      GameButtonState up;
      GameButtonState down;
      GameButtonState left;
      GameButtonState right;
    };
  };
  
};

typedef struct GameInput GameInput;
struct GameInput {
  float dt;
  GameControllerInput player1;
};

typedef struct Entity Entity;
struct Entity {
  V2 pos;
  V2 vel;
  V2 acc;
  
  /* NOTE: For now everything will be recty - This is used for collision and rendering */
  F32 width;
  F32 height;
};


typedef struct GameState GameState;
struct GameState {
  B32 initialized;
  
  B32 is_level_running;
  Entity player;
  Entity opponent;
  Entity ball;
};

#define GAME_UPDATE_AND_RENDER_PROTOTYPE(_name) void _name(GameBackBuffer *back_buffer, GameInput *input, GameState *state)
typedef GAME_UPDATE_AND_RENDER_PROTOTYPE(GameUpdateAndRenderFuncType);

GAME_UPDATE_AND_RENDER_PROTOTYPE(game_update_and_render);

EXTERN_CLOSE /* } */

#endif /* PONG_PLATFORM_H */
