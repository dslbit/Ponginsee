/*
-* TODO LIST:
-*  |_-> Make a rect bound (in-game) of the screen to shake it when players hit the ball (juice)
-*  |_-> +1 effect for points when player hit the ball (juice)
-* 
-*  |_-> Pull out the entity vs arena code? Maybe return a v2 -1 to 1 range to
-*  identify where entity was before going out of bounds
-*
-*  |_-> Simple start menu (Play, Quit) and pause menu (Return, Quit), no text
-*  for now, just a visual representation
-*
-*  |_-> Menu screen (Play, Scoreboard (stores personal records), Settings
-*  (Resolution, Audio, Controls), Quit)
-*  |_-> Pause menu (Main menu, Quit Game)
-*   |_-> Only have pause state for now.
-*  |_-> Save progress? - IDK how crazy this will get, so do it only if it's
-*  needed
-*
-*  |_-> On/Off features (particles, bounce effects, ddp effects, etc.)
-*   |_> Also display that in the F3 (debug) state of the engine/game.
-*  |_-> Figure out text rendering (Bitmap & TrueType)
-*   |_-> Show debug info in-game
-*  |_-> Debug simplified console
-*  |_-> Figure out the sound engine
-*  |_-> Platform-independent: sound output
-*
-* NOTE:
-*  |_-> When I press the pause button when go fullscreen on/off, the 'back buffer' is not rendered.
-* 
*/

#include "pong_base.h"
#include "pong_file_io.h"
#include "pong_math.h"
#include "pong_color.h"
#include "pong_memory.h"
#include "pong_texture_loader.h"
#include "pong_particle.h"
#include "pong_entity.h"
#include "pong_collision.h"
#include "pong_level.h"
#include "pong_platform.h"
#include "pong_renderer.h"

/* NOTE: What if other levels have the same movement code? */
INTERNAL void level_null(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory);
INTERNAL void level_test(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory);
INTERNAL void level_classic(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory);
INTERNAL void level_horizontal_classic(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory);
INTERNAL void level_end(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory);
/*
GAME_UPDATE_AND_RENDER_PROTOTYPE(game_update_and_render) {
*/
void game_update_and_render(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory) {
  GameState *state;
  
  ASSERT(sizeof(GameState) < memory->max_size, L"NOT ENOUGH MEMORY!");
  state = CAST(GameState *) memory->address;
  if (!state->is_initialized) {
    state->is_initialized = TRUE;
    
    /* Platform-independent file I/O test */
    {
      ReadFileResult file = {0};
      
      file = memory->platform_read_entire_file(L"pong.dll");
      memory->platform_free_entire_file(file.data);
    }
    
    state->game_debug_state.is_on = FALSE;
    state->game_debug_state.dt = 0.033333f;
    state->background_color = color_create_from_hex(0x1f1723ff);
    state->background_color_paused = color_create_from_hex(0xefd08180);
    /* NOTE: Maybe this should be moved to the level update function
        initialization place */
    state->game_level.id = LEVEL_ID_NULL;
    state->game_level.is_initialized = FALSE;
    state->game_level.is_running = FALSE;
    state->game_level.time_elapsed = 0.0f;
    state->game_level.time_max = 60.0f;
    state->game_level.min_bounding_rect_x = 0;
    state->game_level.max_bounding_rect_x = back_buffer->width;
    state->game_level.min_bounding_rect_y = 0;
    state->game_level.max_bounding_rect_y = back_buffer->height;
    
    state->player = entity_create(ENTITY_TYPE_PLAYER);
    state->player.color = color_create_from_hex(0x4656a5ff);
    state->player.player_data.score_accumulation = 0.0f;
    
    state->opponent = entity_create(ENTITY_TYPE_PLAYER);
    state->opponent.color = color_create_from_hex(0xf5464cff);
    state->opponent.player_data.score_accumulation = 0.0f;
    
    state->ball = entity_create(ENTITY_TYPE_BLANK);
    state->ball.color = color_create_from_hex(0x3ec54b60);
    state->ball.width = 7;
    state->ball.height = 7;
    state->ball.ball_data.size_multiplier = 1;
  }
  
  /* NOTE: Engine debug mode */
  {
    GameDebugState *debug;
    
    debug = &state->game_debug_state;
    /* debug mode toggle */
    if (input->player1.f3.released) {
      state->game_debug_state.is_on = !state->game_debug_state.is_on;
    }
    
    /* dt change*/
    if (input->player1.minus.released) {
      debug->dt += 0.0023334f;
    }
    if (input->player1.plus.released) {
      debug->dt -= 0.0023334f;
      if (debug->dt < 0) {
        debug->dt = 0;
      }
    }
    
    /* set debug state if it's on */
    if (state->game_debug_state.is_on) {
      input->dt = debug->dt;
    }
  }
  
  /* NOTE: Primitve level selection & pause action */
  {
    /* pause state */
    if(input->player1.start.released && (state->game_level.id != LEVEL_ID_NULL)) {
      if (state->is_paused) {
        state->is_showing_paused_screen = FALSE;
      }
      state->is_paused = !state->is_paused;
    }
    
    /* return to menu state */
    if (input->player1.back.released && (state->game_level.id != LEVEL_ID_NULL)) {
      if (state->is_paused) {
        state->is_showing_paused_screen = FALSE;
        state->is_paused = !state->is_paused;
      }
      state->game_level.is_initialized = FALSE;
      state->game_level.is_running = FALSE;
      state->game_level.id = LEVEL_ID_NULL;
    }
    
    /* press start to play state */
    if (input->player1.start.released && (state->game_level.id == LEVEL_ID_NULL)) {
      state->game_level.is_initialized = FALSE;
      state->game_level.is_running = FALSE;
      state->game_level.id = LEVEL_ID_CLASSIC;
    }
    
    /* Test level - Collision test */
    if (input->player1.aux0.released) {
      state->game_level.is_initialized = FALSE;
      state->game_level.is_running = FALSE;
      state->game_level.id = LEVEL_ID_TEST;
    }
    
    /* Load classic level shortcut */
    if (input->player1.aux1.released) {
      state->game_level.is_initialized = FALSE;
      state->game_level.is_running = FALSE;
      state->game_level.id = LEVEL_ID_CLASSIC;
    }
    
    /* Load horizontal classic level shortcut */
    if (input->player1.aux2.released) {
      state->game_level.is_initialized = FALSE;
      state->game_level.is_running = FALSE;
      state->game_level.id = LEVEL_ID_HORIZONTAL_CLASSIC;
    }
  }
  
  if (state->is_paused) {
    if (!state->is_showing_paused_screen) {
      state->is_showing_paused_screen = TRUE;
      
      /* TODO: Better background clear */
      renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, CAST(F32) back_buffer->width, CAST(F32) back_buffer->height, state->background_color_paused);
    }
    return;
  }
  
  /* NOTE: Level update and render */
  switch (state->game_level.id) {
    case LEVEL_ID_NULL: {
      level_null(back_buffer, input, memory);
    } break;
    
    case LEVEL_ID_TEST: {
      level_test(back_buffer, input, memory);
    } break;
    
    case LEVEL_ID_CLASSIC: {
      level_classic(back_buffer, input, memory);
    } break;
    
    case LEVEL_ID_HORIZONTAL_CLASSIC: {
      level_horizontal_classic(back_buffer, input, memory);
    } break;
    
    case LEVEL_ID_END: {
      level_end(back_buffer, input, memory);
    } break;
    
    default: {
      ASSERT(0, L"Invalid game level ID.");
    }
  }
  
  /* NOTE: Engine debug mode drawing over */
  {
    GameDebugState *debug;
    
    debug = &state->game_debug_state;
    if (debug->is_on) {
      
      /* debug rect indication */
      {
        F32 border_size;
        
        border_size = 3;
        renderer_filled_rect(back_buffer, 0+border_size/2.0f, back_buffer->height/2.0f, border_size, CAST(F32) back_buffer->height, color_create_from_rgba(178, 39, 65, 160)); /* left border */
        
        renderer_filled_rect(back_buffer, back_buffer->width - border_size/2.0f, back_buffer->height/2.0f, border_size, CAST(F32) back_buffer->height, color_create_from_rgba(178, 39, 65, 160)); /* right border */
        
        renderer_filled_rect(back_buffer, back_buffer->width/2.0f, border_size/2.0f, CAST(F32) back_buffer->width - border_size*2, border_size, color_create_from_rgba(178, 39, 65, 160)); /* top border */
        
        renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height-border_size/2.0f, CAST(F32) back_buffer->width - border_size*2, border_size, color_create_from_rgba(178, 39, 65, 160)); /* top border */
      }
      
    }
    
    
  }
}

INTERNAL void level_null(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory) {
  GameState *state;
  
  state = CAST(GameState *) memory->address;
  /* Null level: setup */
  if (!state->game_level.is_initialized) {
    state->game_level.is_initialized = TRUE;
    state->game_level.is_running = TRUE;
    state->game_level.time_elapsed = 0.0f;
    
    v2_zero(&state->opponent.vel);
    v2_zero(&state->player.vel);
    v2_zero(&state->ball.vel);
  }
  
  /* Null level: update */
  {}
  
  /* Null level: rendering */
  {
    /* TODO: Better background clear */
    renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, CAST(F32) back_buffer->width, CAST(F32) back_buffer->height, state->background_color);
  }
}

INTERNAL void level_test(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory) {
  GameState *state;
  
  state = CAST(GameState *) memory->address;
  /* Test level: setup */
  if (!state->game_level.is_initialized) {
    state->game_level.is_initialized = TRUE;
    state->game_level.is_running = TRUE;
    state->game_level.time_elapsed = 0.0f;
    
    state->recty_x = back_buffer->width / 2.0f;
    state->recty_y = back_buffer->height / 2.0f;
    state->recty_width = 100.0f;
    state->recty_height = 75.0f;
    state->recty_rotation = 0.0f;
  }
  
  /* Test level: update */
  {
    state->game_debug_state.accumulated_dt += input->dt;
  }
  
  /* Test level: render */
  {
    renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, CAST(F32) back_buffer->width, CAST(F32) back_buffer->height, state->background_color);
    
    renderer_filled_rotated_rect(back_buffer, state->recty_x, state->recty_y, state->recty_width, state->recty_height, state->recty_rotation, color_create_from_rgba(127, 127, 127, 127));
#if 1
    if (state->game_debug_state.accumulated_dt > 1.0f) {
      state->recty_rotation += input->dt*20.0f;
    }
#endif
  }
}

INTERNAL void level_classic(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory) {
  GameState *state;
  F32 level_bounding_rect_width, level_bounding_rect_height;
  
  state = CAST(GameState *) memory->address;
  level_bounding_rect_width = CAST(F32) (state->game_level.max_bounding_rect_x - state->game_level.min_bounding_rect_x);
  level_bounding_rect_height = CAST(F32) (state->game_level.max_bounding_rect_y - state->game_level.min_bounding_rect_y);
  
  /* Classic level: setup */
  if (!state->game_level.is_initialized) {
    S32 i;
    
    state->game_level.is_initialized = TRUE;
    state->game_level.is_running = TRUE;
    state->game_level.time_elapsed = 0.0f;
    
    v2_zero(&state->player.vel);
    state->player.pos.x = 15;
    state->player.pos.y = level_bounding_rect_height / 2.0f;
    state->player.width = 12;
    state->player.height = 70;
    
    v2_zero(&state->opponent.vel);
    state->opponent.pos.x = level_bounding_rect_width - 15;
    state->opponent.pos.y = level_bounding_rect_height / 2.0f;
    state->opponent.width = 12;
    state->opponent.height = 70;
    
    state->ball.pos.x = level_bounding_rect_width / 2.0f;
    state->ball.pos.y = level_bounding_rect_height / 2.0f;
    {
      F32 vel_x, vel_y;
      S32 sign;
      
      vel_x = random_f32_range(-250.0f, 250.0f);
      sign = SIGN_OF(vel_x);
      vel_x = (ABS(vel_x) < 100) ? (vel_x + (100 * sign)) : (vel_x);
      vel_y = random_f32_range(-110.0f, 110.0f);
      sign = SIGN_OF(vel_y);
      vel_y = (ABS(vel_y) < 50) ? (vel_y + (50 * sign)) : (vel_y);
      state->ball.vel = v2_create(vel_x, vel_y); /* TODO: Check angle between ball and player/opponent, it cannot be too close to 90deg */
    }
    
    for (i = 0; i < ARRAY_COUNT(state->ball.ball_data.trails); ++i) {
      Trail *trail;
      
      trail = &state->ball.ball_data.trails[i];
      trail->pos = state->ball.pos;
      trail->life = 0.0f;
    }
    state->ball.ball_data.particle_system = particle_system_create(memory, FALSE, state->ball.pos, FALSE, 0, 32, 2, 2, state->ball.color);
  }
  
  /* Classic level: update and render */
  if (state->game_level.is_running) { /* level is running */
    /* Clearing some data */
    {}
    
    /* Player movement code */
    {
      Entity *player;
      
      player = &state->player;
      v2_zero(&player->acc);
      
      if (input->player1.up.pressed)    { player->acc.y = -1; }
      if (input->player1.down.pressed)  { player->acc.y = 1;  }
      
      player->acc = v2_mul(player->acc, 5500.0f);
      player->vel = v2_add(player->vel, v2_mul(player->acc, input->dt));
      player->vel = v2_add(player->vel, v2_mul(player->vel, -0.15f));
      player->pos = v2_add(player->pos, v2_mul(player->vel, input->dt));
      
      player->width = 12 - ABS(player->vel.y * 0.00159f);
      player->height = 70 + ABS(player->vel.y * 0.05f);
    }
    
    /* Opponent movement code */
    {
      Entity *opponent;
      
      opponent = &state->opponent;
      v2_zero(&opponent->acc);
      
#if 0
      if ( (state->ball.vel.y < 0) && (state->ball.pos.y < (opponent->pos.y - opponent->height/4.0f)) ) {
        opponent->acc.y = -1.0f;
      } else if ( (state->ball.vel.y > 0) && (state->ball.pos.y > (opponent->pos.y + opponent->height/4.0f)) ) {
        opponent->acc.y = 1.0f;
      }
      
      if ( (ABS(state->ball.pos.y - opponent->pos.y) > opponent->height * 1.5f) ) {
        opponent->acc = v2_mul(opponent->acc, 3500.0f); /* hard: 10500 */
      } else {
        opponent->acc = v2_mul(opponent->acc, 1500.0f); /* hard: 4500, */
      }
      
      opponent->vel = v2_add(opponent->vel, v2_mul(opponent->acc, input->dt));
      opponent->vel = v2_add(opponent->vel, v2_mul(opponent->vel, -0.15f));
      opponent->pos = v2_add(opponent->pos, v2_mul(opponent->vel, input->dt));
      
      
      opponent->width = 12 - ABS(opponent->vel.y * 0.00159f);
      opponent->height = 70 + ABS(opponent->vel.y * 0.05f);
#else
      /* This opponent "movement code" makes the opponent invincible */
      opponent->pos.y = state->ball.pos.y;
#endif
    }
    
    /* Ball movement code - @IMPORTANT: Remember to clamp ball velocity < 'player_width' */
    {
      Entity *ball;
      
      ball = &state->ball;
      v2_zero(&ball->acc);
      
      /* checks if ball velocity mag squared is greater than player width squared - clamp ball velocity, prevent tunneling */
      {
        F32 ball_mag_squared, player_width_squared;
        
        ball_mag_squared = v2_mag_squared( v2_mul(ball->vel, input->dt) );
        player_width_squared = SQUARE(state->player.width);
        if (ball_mag_squared > player_width_squared) {
          ball->vel = v2_add(ball->vel, v2_mul(ball->vel, -0.005f));
          /* TODO: Set ball mag */
#if 0
          ASSERT(0, "Ball is too fast!!!");
#endif
        }
      }
      
      ball->pos = v2_add(ball->pos, v2_mul(ball->vel, input->dt));
      /* NOTE: Update ball trails position */
      {
        U64 trails_count;
        Trail *trail;
        S32 i;
        
        trails_count = ARRAY_COUNT(ball->ball_data.trails);
        ball->ball_data.timer_trail_spawner -= input->dt;
        if (ball->ball_data.timer_trail_spawner < 0) {
          ball->ball_data.timer_trail_spawner = 0.000016f;
          if (ball->ball_data.trails_next >= trails_count) {
            ball->ball_data.trails_next = 0;
          }
          trail = &ball->ball_data.trails[ball->ball_data.trails_next];
          trail->pos = ball->pos;
          trail->angle = v2_angle(ball->vel);
          trail->life = 0.25f * 10.0f;
          ball->ball_data.trails_next++;
        }
        for (i = 0; i < trails_count; ++i) {
          trail = &ball->ball_data.trails[i];
          trail->life -= input->dt*10.0f;
          if (trail->life < 0) trail->life = 0;
        }
      }
      
      /* Update ball size multiplier */
      ball->ball_data.size_multiplier -= input->dt * 2;
      ball->ball_data.size_multiplier = CLAMP(ball->ball_data.size_multiplier, 1.0f, 10.0f);
    }
    
    /* Axis-aligned Collision - @IMPORTANT: make sure it's above the 'clear_background' */
    {
      /* @IDEIA: hit particles effects? */
      
      /* Player VS Arena collision & resolution  */
      {
        S32 player_top, player_bottom;
        
        player_top = CAST(S32) (state->player.pos.y - (state->player.height / 2.0f));
        if (player_top < 0) {
          state->player.pos.y = state->player.height / 2.0f;
          state->player.vel = v2_add(state->player.vel, v2_mul(state->player.vel, -2.5f));
        }
        
        player_bottom = CAST(S32) (state->player.pos.y + (state->player.height / 2.0f));
        if (player_bottom > level_bounding_rect_height) {
          state->player.pos.y = level_bounding_rect_height - state->player.height/2.0f;
          state->player.vel = v2_add(state->player.vel, v2_mul(state->player.vel, -2.5f));
        }
      }
      
      /* Opponent VS Arena collision & resolution  */
      {
        S32 opponent_hit_point_top, opponent_hit_point_bottom;
        
        opponent_hit_point_top = CAST(S32) (state->opponent.pos.y - (state->opponent.height / 2.0f));
        if (opponent_hit_point_top < 0) {
          state->opponent.pos.y = state->opponent.height / 2.0f;
          state->opponent.vel = v2_add(state->opponent.vel, v2_mul(state->opponent.vel, -2.5f));
        }
        
        opponent_hit_point_bottom = CAST(S32) (state->opponent.pos.y + (state->opponent.height / 2.0f));
        if (opponent_hit_point_bottom > level_bounding_rect_height) {
          state->opponent.pos.y = level_bounding_rect_height - state->opponent.height/2.0f;
          state->opponent.vel = v2_add(state->opponent.vel, v2_mul(state->opponent.vel, -0.5f));
        }
      }
      
      /* Ball VS Arena collision & resolution  */
      {
        Entity *ball;
        S32 ball_hit_point_top, ball_hit_point_bottom, ball_hit_point_left, ball_hit_point_right;
        
        ball = &state->ball;
        ball_hit_point_top = CAST(S32) (ball->pos.y - ball->height/2.0f);
        if (ball_hit_point_top < 0) {
          ball->pos.y = ball->height/2.0f;
          ball->vel.y *= -1;
          ball->ball_data.particle_system.is_ready_for_emission = TRUE;
          state->ball.ball_data.size_multiplier = BALL_DEFAULT_SIZE_MULTIPLYER;
        }
        
        ball_hit_point_bottom = CAST(S32) (ball->pos.y + ball->height/2.0f);
        if (ball_hit_point_bottom  > level_bounding_rect_height) {
          ball->pos.y = level_bounding_rect_height - ball->height/2.0f;
          ball->vel.y *= -1;
          ball->ball_data.particle_system.is_ready_for_emission = TRUE;
          state->ball.ball_data.size_multiplier = BALL_DEFAULT_SIZE_MULTIPLYER;
        }
        
        ball_hit_point_left = CAST(S32) (ball->pos.x - ball->width/2.0f);
        if (ball_hit_point_left < 0) {
          ball->pos.x = level_bounding_rect_width/2.0f;
          ball->pos.y = level_bounding_rect_height/2.0f;
          /*state->opponent.player_data.score++;*/
          state->opponent.player_data.score_accumulation = 0.0f;
          state->game_level.is_running = FALSE;
          state->game_level.is_initialized = FALSE;
        }
        
        ball_hit_point_right = CAST(S32) (ball->pos.x + ball->width/2.0f);
        if (ball_hit_point_right > level_bounding_rect_width) {
          ball->pos.x = level_bounding_rect_width/2.0f;
          ball->pos.y = level_bounding_rect_height/2.0f;
          /*state->player.player_data.score++;*/
          state->game_level.is_running = FALSE;
          state->game_level.is_initialized = FALSE;
        }
      }
      
      /* Ball VS Player collision & resolution - Player is to the left side of
the arena */
      {
        B32 is_colliding;
        Entity *player, *ball;
        
        player = &state->player;
        ball = &state->ball;
        is_colliding = collision_aabb_vs_aabb(ball->pos, ball->width, ball->height, player->pos, player->width, player->height);
        if (is_colliding) {
          F32 ball_y_direction;
          
          ball->pos.x = (player->pos.x + player->width/2.0f) + ball->width/2.0f + 1;
          
          if (player->vel.y < 0) {
            ball_y_direction = -1;
            ball->vel.y = ball_y_direction * ABS(ball->vel.y);
          } else if (player->vel.y > 0.001) {
            ball_y_direction = 1;
            ball->vel.y = ball_y_direction * ABS(ball->vel.y);
          }
          
          ball->vel = v2_add(ball->vel, v2_mul(ball->vel, ABS(player->vel.y) * 0.0009f));
          ball->vel.x *= -1;
          ball->ball_data.particle_system.is_ready_for_emission = TRUE;
          state->ball.ball_data.size_multiplier = BALL_DEFAULT_SIZE_MULTIPLYER;
        }
      }
      
      /* Ball VS Opponent collision & resolution - The opponent is to the right side of
the arena */
      {
        B32 is_colliding;
        Entity *opponent, *ball;
        
        opponent = &state->opponent;
        ball = &state->ball;
        is_colliding = collision_aabb_vs_aabb(ball->pos, ball->width, ball->height, opponent->pos, opponent->width, opponent->height);
        if (is_colliding) {
          F32 ball_y_direction;
          
          ball->pos.x = (opponent->pos.x - opponent->width/2.0f) - ball->width/2.0f - 1;
          
          if (opponent->vel.y < 0) {
            ball_y_direction = -1;
            ball->vel.y = ball_y_direction * ABS(ball->vel.y);
          } else if (opponent->vel.y > 0.001) {
            ball_y_direction = 1;
            ball->vel.y = ball_y_direction * ABS(ball->vel.y);
          }
          
          ball->vel = v2_add(ball->vel, v2_mul(ball->vel, ABS(opponent->vel.y) * 0.0009f));
          ball->vel.x *= -1;
          ball->ball_data.particle_system.is_ready_for_emission = TRUE;
          state->ball.ball_data.size_multiplier = BALL_DEFAULT_SIZE_MULTIPLYER;
        }
      }
    }
    
    /* @Test -  particles update after collision */
    {
      state->ball.ball_data.particle_system.pos = state->ball.pos;
      particle_system_update(&state->ball.ball_data.particle_system, input->dt);
    }
    
    /* Re-imagining player score / score progress - Using the arena middle line as a 'load bar' to next levels */
    {
      F32 score_rect_base_height;
      
      state->game_level.time_elapsed += input->dt; /* accumulate dt - Maybe do this somewhere else */
      score_rect_base_height = round_f32(level_bounding_rect_height / state->game_level.time_max);
      state->score_rect_height = CAST(F32) (round_f32_to_s32(state->game_level.time_elapsed) * score_rect_base_height);
      if (state->score_rect_height >= level_bounding_rect_height && (state->game_level.time_elapsed >= state->game_level.time_max)) {
        state->game_level.time_elapsed = 0.0f;
        state->game_level.is_running = FALSE;
        state->game_level.is_initialized = FALSE;
        state->game_level.id = LEVEL_ID_HORIZONTAL_CLASSIC; /* NOTE: Next level - Level transition goes here */
      }
      
      state->score_rect_x = level_bounding_rect_width/2.0f;
      state->score_rect_y = level_bounding_rect_height/2.0f;
      state->score_rect_width = LEVEL_CLASSIC_MIDDLE_LINE_WIDTH;
    }
    
    /* Classic level: rendering */
    {
      GameColor color_middle_line_red, color_middle_line_white, color_score;
      
      color_middle_line_red = color_create_from_hex(0xb22741ff);
      color_middle_line_white = color_create_from_hex(0x8c7f90ff);
      color_score = color_create_from_hex(0xefd081ff);
      
      /* TODO: Better background clear */
      renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, CAST(F32) back_buffer->width, CAST(F32) back_buffer->height, state->background_color);
      
      /* Classic level arena middle line - Red: simulation not running, White: running */
      if (!state->game_level.is_running) {
        renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, LEVEL_CLASSIC_MIDDLE_LINE_WIDTH, CAST(F32) back_buffer->height, color_middle_line_red);
      } else { /* game level is running */
        renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, LEVEL_CLASSIC_MIDDLE_LINE_WIDTH, CAST(F32) back_buffer->height, color_middle_line_white);
      }
      
      /* New score representation */
      renderer_filled_rect(back_buffer, state->score_rect_x, state->score_rect_y, state->score_rect_width, state->score_rect_height, color_score);
      
      /* Ball particle hit */
      {
        renderer_debug_particles(back_buffer, &state->ball.ball_data.particle_system);
      }
      
      /* 'Not so ugly' Ball trail (rect) representation - @IDEIA: change color if it's FAST */
      {
        S32 i;
        GameColor color_trail;
        Trail *trail;
        
        for (i = 0; i < ARRAY_COUNT(state->ball.ball_data.trails); ++i) {
          trail = &state->ball.ball_data.trails[i];
          if (trail->life > 0) {
            color_trail = state->ball.color;
            color_trail.a = (trail->life / 20.0f);
            renderer_filled_rotated_rect(back_buffer, trail->pos.x, trail->pos.y, state->ball.width, state->ball.height, rad_to_deg(trail->angle), color_trail);
          }
        }
        
      }
      
      /* Ball (rect) representation - @IDEIA: change color if it's FAST */
      /* renderer_debug_entity(back_buffer, &state->ball); */
      {
        Entity *b;
        
        b = &state->ball;
        renderer_filled_rect(back_buffer, b->pos.x, b->pos.y, b->width * b->ball_data.size_multiplier, b->height * b->ball_data.size_multiplier, b->color);
      }
      
      /* Player (rect) representation - @IDEIA: change color when moving */
      renderer_debug_entity(back_buffer, &state->player);
      
      /* Opponent (rect) representation - @IDEIA: change color when moving */
      renderer_debug_entity(back_buffer, &state->opponent);
    }
  }
}

INTERNAL void level_horizontal_classic(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory) {
  GameState *state;
  F32 level_bounding_rect_width, level_bounding_rect_height;
  
  state = CAST(GameState *) memory->address;
  level_bounding_rect_width = CAST(F32) (state->game_level.max_bounding_rect_x - state->game_level.min_bounding_rect_x);
  level_bounding_rect_height = CAST(F32) (state->game_level.max_bounding_rect_y - state->game_level.min_bounding_rect_y);
  
  /* Horizontal classic level: setup */
  if (!state->game_level.is_initialized) {
    state->game_level.is_initialized = TRUE;
    state->game_level.is_running = TRUE;
    state->game_level.time_elapsed = 0.0f;
    
    v2_zero(&state->player.vel);
    state->player.pos.x = level_bounding_rect_width / 2.0f;
    state->player.pos.y = level_bounding_rect_height - 15;
    state->player.width = 70;
    state->player.height = 12;
    
    v2_zero(&state->opponent.vel);
    state->opponent.pos.x = level_bounding_rect_width / 2.0f;
    state->opponent.pos.y = 15;
    state->opponent.width = 70;
    state->opponent.height = 12;
    
    state->ball.pos.x = level_bounding_rect_width / 2.0f;
    state->ball.pos.y = level_bounding_rect_height / 2.0f;
    state->ball.vel = v2_create(350.0f, 95.0f);
    
    /* NOTE: Render arena state again? */
  }
  
  /* Horizontal classic level: update */
  if (state->game_level.is_running) { /* level is running */
    /* Player movement code */
    {
      Entity *player;
      
      player = &state->player;
      v2_zero(&player->acc);
      
      if (input->player1.left.pressed)    { player->acc.x = -1; }
      if (input->player1.right.pressed)  { player->acc.x = 1;  }
      
      player->acc = v2_mul(player->acc, 8500.0f);
      player->vel = v2_add(player->vel, v2_mul(player->acc, input->dt));
      player->vel = v2_add(player->vel, v2_mul(player->vel, -0.15f));
      player->pos = v2_add(player->pos, v2_mul(player->vel, input->dt));
      
      player->width = 70 + ABS(player->vel.x * 0.05f);
      player->height = 12 - ABS(player->vel.x * 0.00159f);
    }
    
    /* Opponent movement code */
    {
      Entity *opponent;
      
      opponent = &state->opponent;
      v2_zero(&opponent->acc);
      
#if 0
      /* TODO: This is not working at all for the 'horizontal classic level',
it's a copy-pasta of the classic level */
      if ( (state->ball.vel.y < 0) && (state->ball.pos.y < (opponent->pos.y - opponent->height/4.0f)) ) {
        opponent->acc.y = -1.0f;
      } else if ( (state->ball.vel.y > 0) && (state->ball.pos.y > (opponent->pos.y + opponent->height/4.0f)) ) {
        opponent->acc.y = 1.0f;
      }
      
      if ( (ABS(state->ball.pos.y - opponent->pos.y) > opponent->height * 1.5f) ) {
        opponent->acc = v2_mul(opponent->acc, 3500.0f); /* hard: 10500 */
      } else {
        opponent->acc = v2_mul(opponent->acc, 1500.0f); /* hard: 4500, */
      }
      
      opponent->vel = v2_add(opponent->vel, v2_mul(opponent->acc, input->dt));
      opponent->vel = v2_add(opponent->vel, v2_mul(opponent->vel, -0.15f));
      opponent->pos = v2_add(opponent->pos, v2_mul(opponent->vel, input->dt));
      
      
      opponent->width = 12 - ABS(opponent->vel.y * 0.00159f);
      opponent->height = 70 + ABS(opponent->vel.y * 0.05f);
#else
      /* This opponent "movement code" makes the opponent invincible */
      opponent->pos.x = state->ball.pos.x;
#endif
    }
    
    /* Ball movement code - @IMPORTANT: Remember to clamp ball velocity < 'player_height' */
    {
      Entity *ball;
      
      ball = &state->ball;
      v2_zero(&ball->acc);
      
      /* checks if ball velocity mag squared is greater than player height squared - clamp ball velocity, prevent tunneling */
      {
        F32 ball_mag_squared, player_height_squared;
        
        ball_mag_squared = v2_mag_squared( v2_mul(ball->vel, input->dt) );
        player_height_squared = SQUARE(state->player.height);
        if (ball_mag_squared > player_height_squared) {
          ball->vel = v2_add(ball->vel, v2_mul(ball->vel, -0.005f));
          /* TODO: Set ball mag */
#if 0
          ASSERT(0, "Ball is too fast!!!");
#endif
        }
      }
      
      ball->pos = v2_add(ball->pos, v2_mul(ball->vel, input->dt));
      /* @IDEIA: Ball trail effect? */
    }
    
    /* Axis-aligned Collision - @IMPORTANT: make sure it's above the 'clear_background' */
    {
      /* @IDEIA: hit particles effects? */
      
      /* Player VS Arena collision & resolution */
      {
        S32 player_left, player_right;
        
        player_left = CAST(S32) (state->player.pos.x - (state->player.width / 2.0f));
        if (player_left < 0) {
          state->player.pos.x = state->player.width / 2.0f;
          state->player.vel = v2_add(state->player.vel, v2_mul(state->player.vel, -1.8f));
        }
        
        player_right = CAST(S32) (state->player.pos.x + (state->player.width / 2.0f));
        if (player_right > level_bounding_rect_width) {
          state->player.pos.x = level_bounding_rect_width - state->player.width/2.0f;
          state->player.vel = v2_add(state->player.vel, v2_mul(state->player.vel, -1.8f));
        }
      }
      
      /* Opponent VS Arena collision & resolution */
      {
        S32 opponent_hit_point_left, opponent_hit_point_right;
        
        opponent_hit_point_left = CAST(S32) (state->opponent.pos.x - (state->opponent.width / 2.0f));
        if (opponent_hit_point_left < 0) {
          state->opponent.pos.x = state->opponent.width / 2.0f;
          state->opponent.vel = v2_add(state->opponent.vel, v2_mul(state->opponent.vel, -2.5f));
        }
        
        opponent_hit_point_right = CAST(S32) (state->opponent.pos.x + (state->opponent.width / 2.0f));
        if (opponent_hit_point_right > level_bounding_rect_width) {
          state->opponent.pos.x = level_bounding_rect_width - state->opponent.width/2.0f;
          state->opponent.vel = v2_add(state->opponent.vel, v2_mul(state->opponent.vel, -0.5f));
        }
      }
      
      /* Ball VS Arena collision & resolution */
      {
        Entity *ball;
        S32 ball_hit_point_top, ball_hit_point_bottom, ball_hit_point_left, ball_hit_point_right;
        
        ball = &state->ball;
        
        ball_hit_point_top = CAST(S32) (ball->pos.y - ball->height/2.0f);
        if (ball_hit_point_top < 0) {
          ball->pos.x = level_bounding_rect_width/2.0f;
          ball->pos.y = level_bounding_rect_height/2.0f;
          /*state->player.player_data.score++;*/
          state->opponent.player_data.score_accumulation = 0.0f;
          /*state->game_level.is_running = FALSE;*/
          /* @HACK */
          state->game_level.is_running = FALSE;
          state->game_level.is_initialized = FALSE;
        }
        
        ball_hit_point_bottom = CAST(S32) (ball->pos.y + ball->height/2.0f);
        if (ball_hit_point_bottom  > level_bounding_rect_height) {
          ball->pos.x = level_bounding_rect_width/2.0f;
          ball->pos.y = level_bounding_rect_height/2.0f;
          /*state->opponent.player_data.score++;*/
          state->player.player_data.score_accumulation = 0.0f;
          state->game_level.is_running = FALSE;
          state->game_level.is_initialized = FALSE;
        }
        
        ball_hit_point_left = CAST(S32) (ball->pos.x - ball->width/2.0f);
        if (ball_hit_point_left < 0) {
          ball->pos.x = ball->width/2.0f;
          ball->vel.x *= -1;
        }
        
        ball_hit_point_right = CAST(S32) (ball->pos.x + ball->width/2.0f);
        if (ball_hit_point_right > level_bounding_rect_width) {
          ball->pos.x = level_bounding_rect_width - ball->width/2.0f;
          ball->vel.x *= -1;
        }
      }
      
      /* Ball VS Player collision & resolution */
      {
        B32 is_colliding;
        Entity *player;
        Entity *ball;
        
        player = &state->player;
        ball = &state->ball;
        is_colliding = collision_aabb_vs_aabb(ball->pos, ball->width, ball->height, player->pos, player->width, player->height);
        if (is_colliding) {
          S32 ball_x_direction;
          
          ball->pos.y = (player->pos.y - player->height/2.0f) - (ball->height/2.0f) - 1;
          
          if (player->vel.x < 0) {
            ball_x_direction = -1;
            ball->vel.x = ball_x_direction * ABS(ball->vel.x);
          } else if (player->vel.x > 0.001) {
            ball_x_direction = 1;
            ball->vel.x = ball_x_direction * ABS(ball->vel.x);
          }
          ball->vel = v2_add(ball->vel, v2_mul(ball->vel, ABS(player->vel.x) * 0.0009f));
          ball->vel.y *= -1;
        }
      }
      
      /* Ball VS Opponent collision & resolution */
      {
        B32 is_colliding;
        Entity *opponent;
        Entity *ball;
        
        opponent = &state->opponent;
        ball = &state->ball;
        is_colliding = collision_aabb_vs_aabb(ball->pos, ball->width, ball->height, opponent->pos, opponent->width, opponent->height);
        if (is_colliding) {
          S32 ball_x_direction;
          
          ball->pos.y = (opponent->pos.y + opponent->height/2.0f) + (ball->height/2.0f) + 1;
          
          if (opponent->vel.x < 0) {
            ball_x_direction = -1;
            ball->vel.x = ball_x_direction * ABS(ball->vel.x);
          } else if (opponent->vel.x > 0.001) {
            ball_x_direction = 1;
            ball->vel.x = ball_x_direction * ABS(ball->vel.x);
          }
          ball->vel = v2_add(ball->vel, v2_mul(ball->vel, ABS(opponent->vel.x) * 0.0009f));
          ball->vel.y *= -1;
        }
      }
      
    }
    
    /* Re-imagining player score / score progress - Using the arena middle line as a 'load bar' to next levels */
    {
      F32 score_rect_base_width;
      
      state->game_level.time_elapsed += input->dt; /* accumulate dt - Maybe do this somewhere else */
      score_rect_base_width = round_f32(level_bounding_rect_width / state->game_level.time_max);
      state->score_rect_width = CAST(F32) (round_f32_to_s32(state->game_level.time_elapsed) * score_rect_base_width);
      if (state->score_rect_width >= level_bounding_rect_width && (state->game_level.time_elapsed >= state->game_level.time_max)) {
        state->game_level.time_elapsed = 0.0f;
        state->game_level.is_running = FALSE;
        state->game_level.is_initialized = FALSE;
        state->game_level.id = LEVEL_ID_END; /* @IMPORTANT: End screen */
      }
      
      state->score_rect_x = level_bounding_rect_width/2.0f;
      state->score_rect_y = level_bounding_rect_height/2.0f;
      state->score_rect_height = LEVEL_CLASSIC_MIDDLE_LINE_WIDTH;
    }
    
    /* Horizontal classic level: rendering */
    {
      GameColor color_middle_line_red, color_middle_line_white, color_score;
      
      color_middle_line_red = color_create_from_hex(0xb22741ff);
      color_middle_line_white = color_create_from_hex(0x8c7f90ff);
      color_score = color_create_from_hex(0xefd081ff);
      
      /* TODO: Better background clear */
      renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, CAST(F32) back_buffer->width, CAST(F32) back_buffer->height, state->background_color);
      
      /* Classic level arena middle line - Red: simulation not running, White: running */
      if (!state->game_level.is_running) {
        renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, CAST(F32) back_buffer->width, LEVEL_HORIZONTAL_CLASSIC_MIDDLE_LINE_HEIGHT, color_middle_line_red);
      } else { /* game level is running */
        renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, CAST(F32) back_buffer->width, LEVEL_HORIZONTAL_CLASSIC_MIDDLE_LINE_HEIGHT, color_middle_line_white);
      }
      
      /* New score representation - @IDEIA: Draw the remaining space even though is greater than bounding box? */
      renderer_filled_rect(back_buffer, state->score_rect_x, state->score_rect_y, state->score_rect_width, state->score_rect_height, color_score);
      
      /* Ball (rect) representation - @IDEIA: change color if it's FAST */
      renderer_debug_entity(back_buffer, &state->ball);
      
      /* Player (rect) representation - @IDEIA: change color when moving */
      renderer_debug_entity(back_buffer, &state->player);
      
      /* Opponent (rect) representation - @IDEIA: change color when moving */
      renderer_debug_entity(back_buffer, &state->opponent);
    }
  }
}

/* NOTE: Credits? IDK */
INTERNAL void level_end(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory) {
  GameState *state;
  
  state = CAST(GameState *) memory->address;
  /* End level: setup */
  if (!state->game_level.is_initialized) {
    state->game_level.is_initialized = TRUE;
    state->game_level.is_running = TRUE;
    state->game_level.time_elapsed = 0.0f;
    state->game_level.time_max = 5.0f;
    
    v2_zero(&state->opponent.vel);
    v2_zero(&state->player.vel);
    v2_zero(&state->ball.vel);
  }
  
  /* End level: update */
  {
    state->game_level.time_elapsed += input->dt;
    if (state->game_level.time_elapsed >= state->game_level.time_max) {
      state->game_level.is_initialized = FALSE;
      state->game_level.is_running = FALSE;
      state->game_level.id = LEVEL_ID_NULL;
    }
  }
  
  /* End level: rendering */
  {
    GameColor end_background_color;
    
    end_background_color = color_create_from_hex(0xf09548ff);
    
    /* TODO: Better background clear */
    renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, CAST(F32) back_buffer->width, CAST(F32) back_buffer->height, end_background_color);
  }
}

EXTERN_CLOSE /* } */