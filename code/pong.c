/*
-* TODO:
-*  |_-> Pause states
-*  |_-> Levels
-*
-*  |_-> Simple linear blend of colors
-*  |_-> Simple particle system
-*  |_-> Simple start menu (Play, Quit) and pause menu (Return, Quit), no text
-*  for now, just a visual representation
-*
-*  |_-> Menu screen (Play, Scoreboard (stores personal records), Settings
-*  (Resolution, Audio, Controls), Quit)
-*  |_-> Pause menu (Main menu, Quit Game)
-*  |_-> Save progress? - IDK how crazy this will get, so do it only if it's
-*  needed
-*
-*  |_-> Figure out text rendering
-*   |_-> Show debug info in-game
-*  |_-> Figure out the sound engine
-*  |_-> Platform-independent: game memory, sound output, file I/O
*/

#include "pong_base.h"
#include "pong_math.h"
#include "pong_color.h"
#include "pong_platform.h"
#include "pong_renderer.h"

EXTERN_OPEN /* extern "C" { */

GAME_UPDATE_AND_RENDER_PROTOTYPE(game_update_and_render) {
  if (!state->initialized) {
    state->initialized = TRUE;
    
    state->is_level_running = FALSE;
    state->max_player_score = 3;
    
    state->player = entity_create(ENTITY_TYPE_PLAYER);
    state->player.pos.x = 15; /* player_xoffset */
    state->player.pos.y = (CAST(F32) back_buffer->height) / 2.0f;
    state->player.width = 12;
    state->player.height = 70;
    state->player.color = color_create_from_hex(0x4656a5ff);
    state->player.player_data.score = 0;
    
    state->opponent = entity_create(ENTITY_TYPE_PLAYER);
    state->opponent.pos.x = CAST(F32) (back_buffer->width - 15);
    state->opponent.pos.y = back_buffer->height / 2.0f;
    state->opponent.width = 12;
    state->opponent.height = 70;
    state->opponent.color = color_create_from_hex(0xf5464cff);
    state->opponent.player_data.score = 0;
    
    state->ball = entity_create(ENTITY_TYPE_BLANK);
    state->ball.width = state->ball.height = 9;
    state->ball.color = color_create_from_hex(0x3ec54bff);
    state->ball.pos.x = back_buffer->width / 2.0f;
    state->ball.pos.y = back_buffer->height / 2.0f;
    state->ball.vel = v2_create(-250.0f, -110.0f);
  }
  
  if (!state->is_level_running && input->player1.start.released) {
    state->is_level_running = TRUE;
    state->is_winner_time = FALSE;
    
    /* NOTE: Reset some variables to start the new round */
    state->player.pos.y = (CAST(F32) back_buffer->height) / 2.0f;
    state->player.width = 12;
    state->player.height = 70;
    
    state->opponent.pos.y = back_buffer->height / 2.0f;
    state->opponent.width = 12;
    state->opponent.height = 70;
    
    state->ball.pos.x = back_buffer->width / 2.0f;
    state->ball.pos.y = back_buffer->height / 2.0f;
    state->ball.vel = v2_create(-250.0f, -110.0f);
    
    /* TODO: Render arena state again? */
  }
  
  if (state->is_level_running) {
    /* NOTE: Player movement code */
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
    
    /* NOTE: Opponent movement code */
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
      /* NOTE: This opponent "movement code" makes the opponent invincible */
      opponent->pos.y = state->ball.pos.y;
#endif
    }
    
    /* NOTE: Ball movement code - @IMPORTANT: Remember to clamp ball velocity < 'player_width' */
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
      /* @IDEIA: Ball trail effect? */
    }
    
    /* NOTE: Axis-aligned Collision - @IMPORTANT: make sure it's above the 'clear_background' */
    {
      /* @IDEIA: hit particles effects? */
      
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
        Entity *ball;
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
          state->opponent.player_data.score++;
          state->is_level_running = FALSE;
        }
        
        ball_hit_point_right = CAST(S32) (ball->pos.x + ball->width/2.0f);
        if (ball_hit_point_right > back_buffer->width) {
          ball->pos.x = back_buffer->width/2.0f;
          ball->pos.y = back_buffer->height/2.0f;
          state->player.player_data.score++;
          state->is_level_running = FALSE;
        }
      }
      
      /* Ball VS Player collision */
      {
        Entity *player;
        Entity *ball;
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
        
        if (is_colliding) {
          F32 ball_y_direction;
          
          if (player->vel.y < 0) {
            ball_y_direction = -1;
          } else if (player->vel.y > 0) {
            ball_y_direction = 1;
          } else {
            ball_y_direction = 0;
          }
          
          ball->vel.y = ball_y_direction * ABS(ball->vel.y);
          //ball->vel = v2_mul(ball->vel, CLAMP(player->vel.y * 0.75f, 1.5f, 2.0f));
          ball->vel = v2_add(ball->vel, v2_mul(ball->vel, ABS(player->vel.y) * 0.0009f));
          ball->pos.x = (player->pos.x + player->width/2.0f) + (ball->width/2.0f) + 1;
          ball->vel.x *= -1;
        }
      }
    }
    
    /* Ball VS Opponent collision */
    {
      Entity *opponent;
      Entity *ball;
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
      
      if (is_colliding) {
        
#if 0
        if (ball->pos.y > opponent->pos.y) {
          ball->vel.y = (ball->vel.y > 0) ? -ball->pos.y : ball->pos.y;
        } else {
          ball->vel.y = (ball->vel.y < 0) ? ABS(ball->pos.y) : ball->pos.y;
        }
#endif
        
        ball->pos.x = (opponent->pos.x - opponent->width/2.0f) - (ball->width/2.0f) + 1;
        ball->vel.x *= -1;
      }
    }
    
    /* NOTE: Checking for winner */
    {
      if (state->player.player_data.score == state->max_player_score) {
        state->color_winner = state->player.color;
        state->is_winner_time = TRUE;
      } else if (state->opponent.player_data.score == state->max_player_score) {
        state->color_winner = state->opponent.color;
        state->is_winner_time = TRUE;
      }
    }
  }
  
  /* NOTE: Rendering */
  {
    GameColor color_background, color_middle_line_red, color_middle_line_white;
    
    color_background = color_create_from_hex(0x1f1723ff);
    color_middle_line_red = color_create_from_hex(0xb22741ff);
    color_middle_line_white = color_create_from_hex(0xbcb0b3ff);
    
    if (state->is_winner_time) {
      F32 winner_rect_x, winner_rect_y, winner_rect_width, winner_rect_height;
      
      state->player.player_data.score = 0;
      state->opponent.player_data.score = 0;
      state->is_level_running = FALSE;
      draw_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, CAST(F32) back_buffer->width, CAST(F32) back_buffer->height, color_background);
      winner_rect_width = back_buffer->width/3.0f;
      winner_rect_height = back_buffer->width/3.0f;
      winner_rect_x = back_buffer->width / 2.0f;
      winner_rect_y = back_buffer->height / 2.0f;
      draw_filled_rect(back_buffer, winner_rect_x, winner_rect_y, winner_rect_width, winner_rect_height, state->color_winner);
    } else {
      /* Dirty clear background before drawing, TODO: a proper 'draw_background' */
      draw_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, CAST(F32) back_buffer->width, CAST(F32) back_buffer->height, color_background);
      
      /* Arena middle line - Red: simulation not running, White: running */
#define ARENA_MIDDLE_LINE_WIDTH 3
      if (!state->is_level_running) {
        draw_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, ARENA_MIDDLE_LINE_WIDTH, CAST(F32) back_buffer->height, color_middle_line_red);
      } else {
        draw_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, ARENA_MIDDLE_LINE_WIDTH, CAST(F32) back_buffer->height, color_middle_line_white);
      }
      
      /* NOTE: Drawing players' score - represented as rects, top-centered to the player side */
      {
        S32 score_count, player_score, opponent_score;
        F32 yoffset, xoffset, xpadding, ypadding;
        F32 score_rect_x, score_rect_y, score_rect_width, score_rect_height;
        
        /* Player' score */
        score_rect_width = 9;
        score_rect_height = 9;
        xpadding = 5;
        ypadding = 3;
        xoffset = (back_buffer->width / 2.0f) - ARENA_MIDDLE_LINE_WIDTH - score_rect_width/2.0f - xpadding;
        yoffset = score_rect_height;
        player_score = state->player.player_data.score;
        for (score_count = 1; score_count <= player_score; ++score_count) {
          score_rect_x = xoffset;
          score_rect_y = yoffset;
          xoffset -= score_rect_width + xpadding;
          if ( (score_count != 0) && (score_count % 10 == 0) ) {
            yoffset += score_rect_height + ypadding;
            xoffset = (back_buffer->width / 2.0f) - ARENA_MIDDLE_LINE_WIDTH - score_rect_width/2.0f - xpadding;
          }
          draw_filled_rect(back_buffer, score_rect_x, score_rect_y, score_rect_width, score_rect_height, state->player.color);
        }
        
        /* Opponent' score */
        xoffset = (back_buffer->width / 2.0f) + ARENA_MIDDLE_LINE_WIDTH + score_rect_width/2.0f + xpadding;
        yoffset = score_rect_height;
        opponent_score = state->opponent.player_data.score;
        for (score_count = 1; score_count <= opponent_score; ++score_count) {
          score_rect_x = xoffset;
          score_rect_y = yoffset;
          xoffset += score_rect_width + xpadding;
          if ( (score_count != 0) && (score_count % 10 == 0) ) {
            yoffset += score_rect_height + ypadding;
            xoffset = (back_buffer->width / 2.0f) + ARENA_MIDDLE_LINE_WIDTH + score_rect_width/2.0f + xpadding;
          }
          draw_filled_rect(back_buffer, score_rect_x, score_rect_y, score_rect_width, score_rect_height, state->opponent.color);
        }
      }
      
      /* Player (rect) representation - @IDEIA: change color when moving */
      draw_filled_rect(back_buffer, state->player.pos.x, state->player.pos.y, state->player.width, state->player.height, state->player.color);
      
      /* Opponent (rect) representation - @IDEIA: change color when moving */
      draw_filled_rect(back_buffer, state->opponent.pos.x, state->opponent.pos.y, state->opponent.width, state->opponent.height, state->opponent.color);
      
      /* Ball (rect) representation - @IDEIA: change color if it's FAST */
      draw_filled_rect(back_buffer, state->ball.pos.x, state->ball.pos.y, state->ball.width, state->ball.height, state->ball.color);
    }
  }
}

EXTERN_CLOSE /* } */