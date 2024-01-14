#ifndef PONG_RENDERER_H
#define PONG_RENDERER_H

EXTERN_OPEN /* extern "C" { */

INTERNAL void renderer_pixel(GameBackBuffer *back_buffer, S32 x, S32 y, GameColor color) {
  U32 *pixel;
  U32 color_u32;
  
  if (x >= back_buffer->width || x < 0) return;
  if (y >= back_buffer->height || y < 0) return;
#if 0
  color_u32 = ( (round_f32_to_u32(color.a * 255.0f) << 24) | (round_f32_to_u32(color.r * 255.0f) << 16) | (round_f32_to_u32(color.g * 255.0f) << 8) | (round_f32_to_u32(color.b * 255.0f) << 0) );
#endif
  pixel = CAST(U32 *) ((CAST(U8 *) back_buffer->memory) + (y * back_buffer->stride) + (x * back_buffer->bytes_per_pixel));
  /* linear alpha blending */
  {
    F32 dest_r, dest_g, dest_b;
    F32 r, g, b;
    
    dest_r = ((*pixel) >> 16 & 0xff) / 255.0f;
    dest_g = ((*pixel) >> 8 & 0xff) / 255.0f;
    dest_b = ((*pixel) >> 0 & 0xff) / 255.0f;
    
    r = (1.0f - color.a)*dest_r + color.a*color.r;
    g = (1.0f - color.a)*dest_g + color.a*color.g;
    b = (1.0f - color.a)*dest_b + color.a*color.b;
    
    color_u32 = (0xff << 24) | (round_f32_to_u32(r * 255.0f) << 16) | (round_f32_to_u32(g * 255.0f) << 8) | (round_f32_to_u32(b * 255.0f) << 0);
  }
  *pixel = color_u32; /* AARRGGBB */
}

INTERNAL void renderer_filled_rect(GameBackBuffer *back_buffer, F32 x, F32 y, F32 width, F32 height, GameColor color) {
  S32 i, j;
  S32 start_x, start_y, end_x, end_y;
  U32 *pixel;
  U32 color_u32;
  
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
  
  /* pack floating point color representation to int */
#if 0
  color_u32 = ( (round_f32_to_u32(color.a * 255.0f) << 24) | (round_f32_to_u32(color.r * 255.0f) << 16) | (round_f32_to_u32(color.g * 255.0f) << 8) | (round_f32_to_u32(color.b * 255.0f) << 0) );
#endif
  
  /* start drawing */
  for (i = start_y; i < end_y; ++i) {
    pixel = CAST(U32 *) ((CAST(U8 *) back_buffer->memory) + (i * back_buffer->stride) + (start_x * back_buffer->bytes_per_pixel));
    for (j = start_x; j < end_x; ++j) {
      F32 dest_r, dest_g, dest_b;
      F32 r, g, b;
      
      dest_r = ((*pixel) >> 16 & 0xFF) / 255.0f;
      dest_g = ((*pixel) >> 8 & 0xFF) / 255.0f;
      dest_b = ((*pixel) >> 0 & 0xFF) / 255.0f;
      
      r = (1.0f - color.a)*dest_r + color.a*color.r;
      g = (1.0f - color.a)*dest_g + color.a*color.g;
      b = (1.0f - color.a)*dest_b + color.a*color.b;
      
      color_u32 = (0xff << 24) | (round_f32_to_u32(r * 255.0f) << 16) | (round_f32_to_u32(g * 255.0f) << 8) | (round_f32_to_u32(b * 255.0f) << 0);
      *pixel = color_u32;
      pixel++;
    }
  }
}

INTERNAL INLINE void renderer_debug_entity(GameBackBuffer *back_buffer, Entity *entity) {
  renderer_filled_rect(back_buffer, entity->pos.x, entity->pos.y, entity->width, entity->height, entity->color);
}

INTERNAL INLINE void renderer_debug_particles(GameBackBuffer *back_buffer, ParticleSystem *ps) {
  S32 i;
  Particle *p;
  
  for (i = 0; i < ps->particles_count; ++i) {
    p = &ps->particles[i];
    if (p->life > 0) {
      renderer_filled_rect(back_buffer, p->pos.x, p->pos.y, p->width, p->height, p->color);
    }
  }
}

EXTERN_CLOSE /* } */

#endif /* PONG_RENDERER_H */
