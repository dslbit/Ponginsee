#include "pong_platform.h"

/* TODO: Move to 'renderer' */
EXTERNIZE void draw_pixel(GameBackBuffer *back_buffer, S32 x, S32 y, U32 color) {
  U32 *pixel;
  
  if (x > back_buffer->width || x < 0) return;
  if (y > back_buffer->height || y < 0) return;
  pixel = CAST(U32 *) ((CAST(U8 *) back_buffer->memory) + (y * back_buffer->stride) + (x * back_buffer->bytes_per_pixel));
  *pixel = color; /* AARRGGBB */
}

/* TODO: Move to 'renderer' */
EXTERNIZE void draw_rect(GameBackBuffer *back_buffer, S32 x, S32 y, S32 width, S32 height, U32 color) {
  S32 i, j;
  U32 *pixel;
  
  width = (x + width); /* make width be end_x */
  if (width < 0) width = 0;
  if (width > back_buffer->width) width = back_buffer->width;
  height = (y + height); /* make height be end_y */
  if (height < 0) height = 0;
  if (height > back_buffer->height) height = back_buffer->height;
  if (x < 0) x = 0;
  if (x >  back_buffer->width) x = back_buffer->width;
  if (y < 0) y = 0;
  if (y > back_buffer->height) y = back_buffer->height;
  for (i = y; i < height; ++i) {
    pixel = CAST(U32 *) ((CAST(U8 *) back_buffer->memory) + (i * back_buffer->stride) + (x * back_buffer->bytes_per_pixel));
    for (j = x; j < width; ++j) {
      *pixel = color;
      pixel++;
    }
  }
}

/* TODO: Platform-independent: game memory, sound output, file I/O */
EXTERNIZE GAME_UPDATE_AND_RENDER_PROTOTYPE(game_update_and_render) {
  F32 rect_move_speed;
  
  rect_move_speed = 2500.0f * input->dt;
  if (input->player1.up.pressed)    { state->player_y -= rect_move_speed; }
  if (input->player1.down.pressed)  { state->player_y += rect_move_speed; }
  if (input->player1.left.pressed)  { state->player_x -= rect_move_speed; }
  if (input->player1.right.pressed) { state->player_x += rect_move_speed; }
  
  /* Dirty clear background before drawing, TODO: a proper 'draw_background' */
  draw_rect(back_buffer, 0, 0, back_buffer->width, back_buffer->height, 0x00000000);
  
  /* Player (rect) representation */
  draw_rect(back_buffer, 50 + CAST(int) state->player_x, 50 + CAST(int) state->player_y, 80, 45, 0xFF0000FF);
}