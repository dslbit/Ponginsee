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

INTERNAL void renderer_filled_rotated_rect(GameBackBuffer *back_buffer, F32 x, F32 y, F32 width, F32 height, F32 angle, GameColor color) {
  /* start drawing - (normal) filled rect */
  if (angle == 0.0f || angle == 360.0f) {
    renderer_filled_rect(back_buffer, x, y, width, height, color);
  } else { // rotated rect
    F32 radians;
    F32 sin, cos;
    V2 x_axis, y_axis;
    M2 rotation_matrix;
    Rect2 rect;
    S32 k;
    V2 rect_pos;
    S32 min_bound_x, min_bound_y;
    S32 max_bound_x, max_bound_y;
    
    radians = angle * (PI / 180.0f);
    sin = sinf(radians);
    cos = cosf(radians);
    x_axis = v2_create(cos, sin);
    y_axis = v2_create(-sin, cos);
    rotation_matrix = m2_create(x_axis.x, y_axis.x, x_axis.y, y_axis.y);
    
    /* rect vertex */
    {
      /* anchor: rect center */
      V2 rect_min = v2_create(-width/2.0f, -height/2.0f);
      V2 rect_max = v2_create(width/2.0f, height/2.0f);
      rect = rect2_create(rect_min, rect_max);
    }
    
    rect_pos = v2_create(x, y);
    min_bound_x = max_bound_x = min_bound_y = max_bound_y = 0;
    for (k = 0; k < 4; ++k) {
      rect.p[k] = m2_mul_v2(rotation_matrix, rect.p[k]); /* rect vertex rotation */
      rect.p[k] = v2_add(rect.p[k], rect_pos); /* add offset to rotated vertex */
      
      /* finding min & max bounding values */
      {
        S32 value_x, value_y;
        value_x = truncate_f32(rect.p[k].x);
        value_y = truncate_f32(rect.p[k].y);
        if (value_x < min_bound_x) min_bound_x = value_x;
        if (value_x > max_bound_x) max_bound_x = value_x;
        if (value_y < min_bound_y) min_bound_y = value_y;
        if (value_y > max_bound_y) max_bound_y = value_y;
      }
    }
    
    /* fill rotated rect */
    {
      S32 i, j;
      V2 axis_1, axis_2;
      F32 axis_1_mag, axis_2_mag;
      F32 rel_pixel_px, rel_pixel_py;
      F32 projection_1, projection_2;
      U32 *row;
      U32 *pixel;
      S32 stride;
      
      min_bound_x = CLAMP(min_bound_x, 0, back_buffer->width);
      max_bound_x = CLAMP(max_bound_x, 0, back_buffer->width);
      min_bound_y = CLAMP(min_bound_y, 0, back_buffer->height);
      max_bound_y = CLAMP(max_bound_y, 0, back_buffer->height);
      
      axis_1 = v2_sub(rect.p[1], rect.p[0]);
      axis_1_mag = v2_mag(axis_1);
      axis_1 = v2_norm(axis_1);
      
      axis_2 = v2_sub(rect.p[3], rect.p[0]);
      axis_2_mag = v2_mag(axis_2);
      axis_2 = v2_norm(axis_2);
      
      stride = back_buffer->width;
      row = CAST(U32 *) back_buffer->memory + (min_bound_y * stride) + min_bound_x;
      pixel = row;
      for (i = min_bound_y; i < max_bound_y; ++i) {
        for (j = min_bound_x; j < max_bound_x; ++j) {
          rel_pixel_px = CAST(F32) j - rect.p[0].x;
          rel_pixel_py = CAST(F32) i - rect.p[0].y;
          projection_1 = (rel_pixel_px * axis_1.x) + (rel_pixel_py * axis_1.y);
          projection_2 = (rel_pixel_px * axis_2.x) + (rel_pixel_py * axis_2.y);
          if ( (projection_1 >= 0 && projection_1 <= axis_1_mag) && (projection_2 >= 0 && projection_2 <= axis_2_mag) ) {
            F32 dest_r, dest_g, dest_b;
            F32 r, g, b;
            U32 color_u32;
            
            dest_r = ((*pixel) >> 16 & 0xFF) / 255.0f;
            dest_g = ((*pixel) >> 8 & 0xFF) / 255.0f;
            dest_b = ((*pixel) >> 0 & 0xFF) / 255.0f;
            
            r = (1.0f - color.a)*dest_r + color.a*color.r;
            g = (1.0f - color.a)*dest_g + color.a*color.g;
            b = (1.0f - color.a)*dest_b + color.a*color.b;
            
            color_u32 = (0xff << 24) | (round_f32_to_u32(r * 255.0f) << 16) | (round_f32_to_u32(g * 255.0f) << 8) | (round_f32_to_u32(b * 255.0f) << 0);
            *pixel = color_u32;
          }
          ++pixel;
        }
        row += stride;
        pixel = row;
      }
    }
    
    /* debug rect vertex drawing */
#if 0
    {
      GameColor vertex_color = color_create_from_rgba(127, 12, 12, 255);
      renderer_filled_rect(back_buffer, rect.p[0].x, rect.p[0].y, 5, 5, vertex_color);
      renderer_filled_rect(back_buffer, rect.p[1].x, rect.p[1].y, 5, 5, vertex_color);
      renderer_filled_rect(back_buffer, rect.p[2].x, rect.p[2].y, 5, 5, vertex_color);
      renderer_filled_rect(back_buffer, rect.p[3].x, rect.p[3].y, 5, 5, vertex_color);
    }
#endif
    
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
