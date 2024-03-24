#ifndef PONG_PLATFORM_H
#define PONG_PLATFORM_H

#define GAME_DEFAULT_DATA_RELATIVE_PATH L"..\\data\\"
#define GAME_BMP_FONT_DEFAULT L"font_default_8x16.bmp"
#define GAME_INPUT_TEXT_STREAM_LENGTH (128)


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
  B32 enabled;
  union {
    GameButtonState buttons[19];
    struct {
      /* engine/game controls */
      GameButtonState start; /* or enter */
      GameButtonState back; /* also used for menu (escape key / select on controller) */
      GameButtonState up;
      GameButtonState down;
      GameButtonState left;
      GameButtonState right;
      
      GameButtonState f9; /* toggle engine console */
      GameButtonState f3; /* toggle debug mode */
      GameButtonState plus; /* debug mode +dt */
      GameButtonState minus; /* debug mode -dt */
      
      /* used to quick-swap game levels */
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

typedef struct GameInputTextStream GameInputTextStream;
struct GameInputTextStream {
  S8 stream[GAME_INPUT_TEXT_STREAM_LENGTH]; /* samples are cleared at the end of every frame */
  S32 last_index;
};

typedef struct GameInput GameInput;
struct GameInput {
  float dt; /* last frame seconds elapsed */
  V2 mouse_pos;
  GameControllerInput player1;
  GameInputTextStream text_stream;
};

typedef struct GameBitmapFont GameBitmapFont;
struct GameBitmapFont {
  S32 glyph_width;
  S32 glyph_height;
  /* TODO: Lookup table */
  Texture bmp;
};

typedef struct GameDebugState GameDebugState;
struct GameDebugState {
  B32 is_on;
  F32 dt;
  F32 dt_original;
  B32 is_particles_on;
  U32 count_particles;
  B32 is_trails_on;
  U32 count_trails;
  B32 is_ddp_effects_on;
  U32 count_ddp_effects_per_entity;
  F64 accumulated_dt;
};

typedef struct GameState GameState;
struct GameState {
  B32 is_initialized;
  B32 is_paused;
  B32 is_showing_paused_screen;
  U32 random_seed;
  GameConsoleState game_console_state;
  GameDebugState game_debug_state;
  GameMemory game_memory;
  GameLevel game_level;
  F32 score_rect_x;
  F32 score_rect_y;
  F32 score_rect_width;
  F32 score_rect_height;
  
  /* NOTE: TEMPORARY - Test level data */
  GameBitmapFont bmp_font_default;
  GameColor text_default_color;
  GameColor text_color_red;
  GameColor text_color_green;
  /* --- */
  
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
