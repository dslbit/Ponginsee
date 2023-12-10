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
  start_x = round_f32_to_s32(x);
  start_y = round_f32_to_s32(y);
  end_x   = round_f32_to_s32(x + width);
  end_y   = round_f32_to_s32(y + height);
  
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
  F32 rect_move_speed;
  
  rect_move_speed = 100.0f * input->dt;
  if (input->player1.up.pressed)    { state->player_y -= rect_move_speed; }
  if (input->player1.down.pressed)  { state->player_y += rect_move_speed; }
  if (input->player1.left.pressed)  { state->player_x -= rect_move_speed; }
  if (input->player1.right.pressed) { state->player_x += rect_move_speed; }
  
  /* Dirty clear background before drawing, TODO: a proper 'draw_background' */
  draw_rect(back_buffer, 0.0f, 0.0f, CAST(F32) back_buffer->width, CAST(F32) back_buffer->height, 1.0f, 0.0f, 1.0f);
  
  /* Player (rect) representation */
  draw_rect(back_buffer, 50 + state->player_x, 50 + state->player_y, 15, 15, 0.364705f, 0.464705f, 0.964705f);
}