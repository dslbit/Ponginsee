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

typedef enum EntityType EntityType;
enum EntityType {
  ENTITY_TYPE_BLANK,
  ENTITY_TYPE_PLAYER,
  
  ENTITY_COUNT
};

typedef struct EntityPlayer EntityPlayer;
struct EntityPlayer {
  U32 score;
};

typedef struct Entity Entity;
struct Entity {
  V2 pos;
  V2 vel;
  V2 acc;
  
  /* NOTE: For now everything will be recty - This is used for collision and rendering */
  F32 width;
  F32 height;
  
  EntityType entity_type;
  union {
    EntityPlayer player_data;
  };
};

typedef struct GameState GameState;
struct GameState {
  B32 initialized;
  
  B32 is_level_running;
  Entity player;
  Entity opponent;
  Entity ball;
};

INTERNAL Entity entity_create(EntityType type) {
  Entity result = {0};
  
  result.entity_type = type;
  return result;
}

#define GAME_UPDATE_AND_RENDER_PROTOTYPE(_name) void _name(GameBackBuffer *back_buffer, GameInput *input, GameState *state)
typedef GAME_UPDATE_AND_RENDER_PROTOTYPE(GameUpdateAndRenderFuncType);

GAME_UPDATE_AND_RENDER_PROTOTYPE(game_update_and_render);

EXTERN_CLOSE /* } */

#endif /* PONG_PLATFORM_H */
