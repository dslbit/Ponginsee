#include "pong_base.h"
#include "pong_math.h"
#include "pong_platform.h"

/* TODO: Move to 'renderer' */
EXTERNIZE void draw_pixel(GameBackBuffer *back_buffer, S32 x, S32 y, U32 color) {
  U32 *pixel;
  
  if (x > back_buffer->width || x < 0) return;
  if (y > back_buffer->height || y < 0) return;
  pixel = CAST(U32 *) ((CAST(U8 *) back_buffer->memory) + (y * back_buffer->stride) + (x * back_buffer->bytes_per_pixel));
  *pixel = color; /* AARRGGBB */
}

INTERNAL S32 round_f32_to_s32(F32 value) {
  S32 result;
  
  result = CAST(S32) (value + 0.5f);
  return result;
}

INTERNAL U32 round_f32_to_u32(F32 value) {
  U32 result;
  
  result = CAST(U32) (value + 0.5f);
  return result;
}

/* TODO: Move to 'renderer' ; color struct ; make_rgb_to_float/make_hex_to_float? */
EXTERNIZE void draw_rect(GameBackBuffer *back_buffer, F32 x, F32 y, F32 width, F32 height, F32 R, F32 G, F32 B) {
  S32 i, j;
  S32 start_x, start_y, end_x, end_y;
  U32 *pixel;
  U32 color;
  
  /* round to int */
  start_x = round_f32_to_s32(x - width/2.0f);
  start_y = round_f32_to_s32(y - height/2.0f);
  end_x   = round_f32_to_s32(start_x + width);
  end_y   = round_f32_to_s32(start_y + height);
  
  /* clip to buffer */
  if (start_x < 0) start_x = 0;
  if (start_x >  back_buffer->width) start_x = back_buffer->width;
  if (start_y < 0) start_y = 0;
  if (start_y > back_buffer->height) start_y = back_buffer->height;
  if (end_x < 0) end_x = 0;
  if (end_x > back_buffer->width) end_x = back_buffer->width;
  if (end_y < 0) end_y = 0;
  if (end_y > back_buffer->height) end_y = back_buffer->height;
  
  /* pack floating point color representation to int - TODO: alpha channel */
  color = ( (round_f32_to_u32(R * 255.0f) << 16) | (round_f32_to_u32(G * 255.0f) << 8) | (round_f32_to_u32(B * 255.0f) << 0) );
  
  /* start drawing */
  for (i = start_y; i < end_y; ++i) {
    pixel = CAST(U32 *) ((CAST(U8 *) back_buffer->memory) + (i * back_buffer->stride) + (start_x * back_buffer->bytes_per_pixel));
    for (j = start_x; j < end_x; ++j) {
      *pixel = color;
      pixel++;
    }
  }
}

/* TODO: Platform-independent: game memory, sound output, file I/O */
EXTERNIZE GAME_UPDATE_AND_RENDER_PROTOTYPE(game_update_and_render) {
  if (!state->initialized) {
    state->initialized = TRUE;
    
    state->player.pos.x = 15; /* player_xoffset */
    state->player.pos.y = (CAST(F32) back_buffer->height) / 2.0f;
    
    state->opponent.pos.x = CAST(F32) (back_buffer->width - 15);
    state->opponent.pos.y = back_buffer->height / 2.0f;
    
    state->ball.width = state->ball.height = 9;
    state->ball.pos.x = back_buffer->width / 2.0f;
    state->ball.pos.y = back_buffer->height / 2.0f;
    state->ball.vel = v2_create(-555.0f, -210.0f);
  }
  
  /* NOTE: Player movement code */
  {
    Player *player;
    
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
  
  /* NOTE: Opponent movement code */
  {
    Opponent *opponent;
    
    opponent = &state->opponent;
    v2_zero(&opponent->acc);
    
    /* NOTE: For now, opponent movement code makes the opponent almost invincible */
    if ( (state->ball.vel.y < 0) && (state->ball.pos.y < (opponent->pos.y - opponent->height/5.0f)) ) {
      opponent->acc.y = -1.0f;
    } else if ( (state->ball.vel.y > 0) && (state->ball.pos.y > (opponent->pos.y + opponent->height/5.0f)) ) {
      opponent->acc.y = 1.0f;
    }
    
    if ( (ABS(state->ball.pos.y - opponent->pos.y) > opponent->height) ) {
      opponent->acc = v2_mul(opponent->acc, 25500.0f);
    } else {
      opponent->acc = v2_mul(opponent->acc, 3500.0f);
    }
    
    opponent->vel = v2_add(opponent->vel, v2_mul(opponent->acc, input->dt));
    opponent->vel = v2_add(opponent->vel, v2_mul(opponent->vel, -0.15f));
    opponent->pos = v2_add(opponent->pos, v2_mul(opponent->vel, input->dt));
    
    opponent->width = 12 - ABS(opponent->vel.y * 0.00159f);
    opponent->height = 70 + ABS(opponent->vel.y * 0.05f);
  }
  
  /* NOTE: Ball movement code - TODO: Remember to clamp ball velocity < 'player_width' */
  {
    Ball *ball;
    
    ball = &state->ball;
    v2_zero(&ball->acc);
    
    /* TODO: Calc ball acceleration, maybe use an 'applied_forces' vector ; maybe after player hit? */
    
    /* checks if ball velocity mag squared is greater than player width squared - clamp ball velocity, prevent tunneling */
    {
      F32 ball_mag_squared, player_width_squared;
      
      ball_mag_squared = v2_mag_squared( v2_mul(ball->vel, input->dt) );
      player_width_squared = SQUARE(state->player.width);
      if (ball_mag_squared > player_width_squared) {
        /* TODO: Set ball mag */
#if 0
        ASSERT(0, "Ball is too fast!!!");
#endif
      }
    }
    
    ball->pos = v2_add(ball->pos, v2_mul(ball->vel, input->dt));
    /* TODO: Ball trail effect? */
  }
  
  /* NOTE: Axis-aligned Collision - @IMPORTANT: make sure it's above the 'clear_background' */
  {
    /* TODO: hit particles effects? */
    
    /* Player VS Arena collision */
    {
      S32 player_top, player_bottom;
      
      player_top = CAST(S32) (state->player.pos.y - (state->player.height / 2.0f));
      if (player_top < 0) {
        state->player.pos.y = state->player.height / 2.0f;
        state->player.vel = v2_add(state->player.vel, v2_mul(state->player.vel, -2.5f));
      }
#if 0
      draw_rect(back_buffer, state->player.pos.x, CAST(F32) player_top, 5, 5, 1.0f, 0.0f, 1.0f); /* hit point debug draw */
#endif
      
      player_bottom = CAST(S32) (state->player.pos.y + (state->player.height / 2.0f));
      if (player_bottom > back_buffer->height) {
        state->player.pos.y = (CAST(F32) back_buffer->height) - state->player.height / 2.0f;
        state->player.vel = v2_add(state->player.vel, v2_mul(state->player.vel, -2.5f));
      }
#if 0
      draw_rect(back_buffer, state->player.pos.x, CAST(F32) player_bottom, 5, 5, 1.0f, 0.0f, 1.0f); /* hit point debug draw */
#endif
    }
    
    /* Opponent VS Arena collision */
    {
      S32 opponent_hit_point_top, opponent_hit_point_bottom;
      
      opponent_hit_point_top = CAST(S32) (state->opponent.pos.y - (state->opponent.height / 2.0f));
      if (opponent_hit_point_top < 0) {
        state->opponent.pos.y = state->opponent.height / 2.0f;
        state->opponent.vel = v2_add(state->opponent.vel, v2_mul(state->opponent.vel, -2.5f));
      }
      
      opponent_hit_point_bottom = CAST(S32) (state->opponent.pos.y + (state->opponent.height / 2.0f));
      if (opponent_hit_point_bottom > back_buffer->height) {
        state->opponent.pos.y = (CAST(F32) back_buffer->height) - state->opponent.height / 2.0f;
        state->opponent.vel = v2_add(state->opponent.vel, v2_mul(state->opponent.vel, -0.5f));
      }
    }
    
    /* Ball VS Arena collision */
    {
      Ball *ball;
      S32 ball_hit_point_top, ball_hit_point_bottom, ball_hit_point_left, ball_hit_point_right;
      
      ball = &state->ball;
      ball_hit_point_top = CAST(S32) (ball->pos.y - ball->height/2.0f);
      if (ball_hit_point_top < 0) {
        ball->pos.y = ball->height/2.0f;
        ball->vel.y *= -1;
      }
      
      ball_hit_point_bottom = CAST(S32) (ball->pos.y + ball->height/2.0f);
      if (ball_hit_point_bottom  > back_buffer->height) {
        ball->pos.y = CAST(F32) (back_buffer->height - ball->height/2.0f);
        ball->vel.y *= -1;
      }
      
      ball_hit_point_left = CAST(S32) (ball->pos.x - ball->width/2.0f);
      if (ball_hit_point_left < 0) {
        ball->pos.x = back_buffer->width/2.0f;
        ball->pos.y = back_buffer->height/2.0f;
        /* TODO: Player score++ */
      }
      
      ball_hit_point_right = CAST(S32) (ball->pos.x + ball->width/2.0f);
      if (ball_hit_point_right > back_buffer->width) {
        ball->pos.x = back_buffer->width/2.0f;
        ball->pos.y = back_buffer->height/2.0f;
        /* TODO: Opponent score++ */
      }
    }
    
    /* Ball VS Player collision */
    {
      Player *player;
      Ball *ball;
      S32 player_hit_point_top, player_hit_point_bottom, player_hit_point_left, player_hit_point_right;
      S32 ball_hit_point_top, ball_hit_point_bottom, ball_hit_point_left, ball_hit_point_right;
      B32 is_colliding;
      
      is_colliding = FALSE;
      
      player = &state->player;
      player_hit_point_top = CAST(S32) (player->pos.y - player->height/2.0f);
      player_hit_point_bottom = CAST(S32) (player->pos.y + player->height/2.0f);
      player_hit_point_left = CAST(S32) (player->pos.x - player->width/2.0f);
      player_hit_point_right = CAST(S32) (player->pos.x + player->width/2.0f);
      
      ball = &state->ball;
      ball_hit_point_top = CAST(S32) (ball->pos.y - ball->height/2.0f);
      ball_hit_point_bottom = CAST(S32) (ball->pos.y + ball->height/2.0f);
      ball_hit_point_left = CAST(S32) (ball->pos.x - ball->width/2.0f);
      ball_hit_point_right = CAST(S32) (ball->pos.x + ball->width/2.0f);
      
      /* AABB vs AABB - Ball vs Player */
      if ( (ball_hit_point_left >= player_hit_point_left) && (ball_hit_point_left <= player_hit_point_right) && (ball_hit_point_top >= player_hit_point_top) && (ball_hit_point_bottom <= player_hit_point_bottom) ) {
        is_colliding = TRUE;
      } else if ( (ball_hit_point_right >= player_hit_point_left) && (ball_hit_point_right <= player_hit_point_right) && (ball_hit_point_top >= player_hit_point_top) && (ball_hit_point_bottom <= player_hit_point_bottom) ) {
        is_colliding = TRUE;
      }
      
      /* TODO: Move ball up/down based on player velocity and hit point */
      if (is_colliding) {
        if (ball->pos.y > player->pos.y) {
          ball->vel.y = (ball->vel.y > 0) ? -ball->pos.y : ball->pos.y;
        } else {
          ball->vel.y = (ball->vel.y < 0) ? ABS(ball->pos.y) : ball->pos.y;
        }
        
        ball->pos.x = (player->pos.x + player->width/2.0f) + (ball->width/2.0f) + 1;
        ball->vel.x *= -1;
      }
    }
    
    /* Ball VS Opponent collision */
    {
      Opponent *opponent;
      Ball *ball;
      S32 opponent_hit_point_top, opponent_hit_point_bottom, opponent_hit_point_left, opponent_hit_point_right;
      S32 ball_hit_point_top, ball_hit_point_bottom, ball_hit_point_left, ball_hit_point_right;
      B32 is_colliding;
      
      is_colliding = FALSE;
      
      opponent = &state->opponent;
      opponent_hit_point_top = CAST(S32) (opponent->pos.y - opponent->height/2.0f);
      opponent_hit_point_bottom = CAST(S32) (opponent->pos.y + opponent->height/2.0f);
      opponent_hit_point_left = CAST(S32) (opponent->pos.x - opponent->width/2.0f);
      opponent_hit_point_right = CAST(S32) (opponent->pos.x + opponent->width/2.0f);
      
      ball = &state->ball;
      ball_hit_point_top = CAST(S32) (ball->pos.y - ball->height/2.0f);
      ball_hit_point_bottom = CAST(S32) (ball->pos.y + ball->height/2.0f);
      ball_hit_point_left = CAST(S32) (ball->pos.x - ball->width/2.0f);
      ball_hit_point_right = CAST(S32) (ball->pos.x + ball->width/2.0f);
      
      /* AABB vs AABB - Ball vs Opponent */
      if ( (ball_hit_point_left >= opponent_hit_point_left) && (ball_hit_point_left <= opponent_hit_point_right) && (ball_hit_point_top >= opponent_hit_point_top) && (ball_hit_point_bottom <= opponent_hit_point_bottom) ) {
        is_colliding = TRUE;
      } else if ( (ball_hit_point_right >= opponent_hit_point_left) && (ball_hit_point_right <= opponent_hit_point_right) && (ball_hit_point_top >= opponent_hit_point_top) && (ball_hit_point_bottom <= opponent_hit_point_bottom) ) {
        is_colliding = TRUE;
      }
      
      /* TODO: Move ball up/down based on opponent velocity and hit point */
      if (is_colliding) {
        if (ball->pos.y > opponent->pos.y) {
          ball->vel.y = (ball->vel.y > 0) ? -ball->pos.y : ball->pos.y;
        } else {
          ball->vel.y = (ball->vel.y < 0) ? ABS(ball->pos.y) : ball->pos.y;
        }
        
        ball->pos.x = (opponent->pos.x - opponent->width/2.0f) - (ball->width/2.0f) + 1;
        ball->vel.x *= -1;
      }
    }
  }
  
  /* TODO: Convert hex/rgb colors to float version */
  /* NOTE: Rendering */
  {
    /* Dirty clear background before drawing, TODO: a proper 'draw_background' */
    draw_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, CAST(F32) back_buffer->width, CAST(F32) back_buffer->height, 31.0f/255.0f, 23.0f/255.0f, 35.0f/255);
    
    /* Player (rect) representation */
    draw_rect(back_buffer, state->player.pos.x, state->player.pos.y, state->player.width, state->player.height, 70.0f/255.0f, 86.0f/255.0f, 165.0f/255.0f);
    
    /* Opponent (rect) representation */
    draw_rect(back_buffer, state->opponent.pos.x, state->opponent.pos.y, state->opponent.width, state->opponent.height, 245.0f/255.0f, 70.0f/255.0f, 76.0f/255.0f);
    
    /* Ball (rect) representation */
    draw_rect(back_buffer, state->ball.pos.x, state->ball.pos.y, state->ball.width, state->ball.height, 62.0f/255.0f, 197.0f/255.0f, 75.0f/255.0f);
  }
}
