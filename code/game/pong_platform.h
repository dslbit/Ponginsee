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
    GameButtonState buttons[19];
    struct {
      GameButtonState start;
      GameButtonState back; /* also used for menu (escape key / select on controller) */
      GameButtonState up;
      GameButtonState down;
      GameButtonState left;
      GameButtonState right;
      
      GameButtonState f3;
      GameButtonState plus;
      GameButtonState minus;
      
      GameButtonState aux0;
      GameButtonState aux1;
      GameButtonState aux2;
      GameButtonState aux3;
      GameButtonState aux4;
      GameButtonState aux5;
      GameButtonState aux6;
      GameButtonState aux7;
      GameButtonState aux8;
      GameButtonState aux9;
    };
  };
  
};

typedef struct GameInput GameInput;
struct GameInput {
  float dt; /* last frame seconds elapsed */
  V2 mouse_pos;
  GameControllerInput player1;
};

typedef struct GameDebugState GameDebugState;
struct GameDebugState {
  B32 is_on;
  F32 dt;
  F32 accumulated_dt;
};

typedef struct GameState GameState;
struct GameState {
  B32 is_initialized;
  B32 is_paused;
  B32 is_showing_paused_screen;
  U32 random_seed;
  GameDebugState game_debug_state;
  GameMemory game_memory;
  GameLevel game_level;
  F32 score_rect_x;
  F32 score_rect_y;
  F32 score_rect_width;
  F32 score_rect_height;
  
  /* NOTE: TEMPORARY - Test level data */
  Texture temp_texture;
  F32 temp_texture_scale;
  
  GameColor background_color;
  GameColor background_color_paused;
  /* @IMPORTANT: In a more generalized view, 'game_level' should store the 
entities - Right? */
  Entity player;
  Entity opponent;
  Entity ball;
};

typedef void GameUpdateAndRenderFuncType(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory);
void game_update_and_render(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory);


EXTERN_CLOSE /* } */

#endif /* PONG_PLATFORM_H */
