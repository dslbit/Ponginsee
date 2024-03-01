#ifndef PONG_RENDERER_H
#define PONG_RENDERER_H

EXTERN_OPEN /* extern "C" { */

INTERNAL INLINE void renderer_pixel(GameBackBuffer *back_buffer, S32 x, S32 y, GameColor color) {
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
  if (angle == 0.0f || angle == 360.0f) { /* % 360 == 0? */
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
    
    radians = deg_to_rad(angle);
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
      renderer_filled_rotated_rect(back_buffer, p->pos.x, p->pos.y, p->width, p->height, rad_to_deg(p->angle), p->color);
    }
  }
}

INTERNAL void renderer_debug_texture(GameBackBuffer *back_buffer, Texture texture, F32 start_x, F32 start_y) {
  S32 x, y;
  S32 j, i;
  U32 *pixel;
  U32 *bmp_pixel;
  F32 r, g, b, a;
  F32 dest_r, dest_g, dest_b;
  
  x = round_f32_to_s32(start_x);
  y = round_f32_to_s32(start_y);
  for (i = y; (i < (y + texture.height)) && (i < back_buffer->height); ++i) {
    pixel = (CAST(U32 *) back_buffer->memory) + (i * back_buffer->width) + x;
    bmp_pixel = CAST(U32 *) texture.data + ((texture.width * texture.height) - texture.width * (i - y + 1)); /* y top-left */
    for (j = x; (j < (x + texture.width)) && (j < back_buffer->width); ++j) {
      r = (*bmp_pixel >> 16 & 0xFF) / 255.0f;
      g = (*bmp_pixel >> 8 & 0xFF) / 255.0f;
      b = (*bmp_pixel >> 0 & 0xFF) / 255.0f;
      a = (*bmp_pixel >> 24 & 0xFF) / 255.0f;
      
      dest_r = (*pixel >> 16 & 0xFF) / 255.0f;
      dest_g = (*pixel >> 8 & 0xFF) / 255.0f;
      dest_b = (*pixel >> 0 & 0xFF) / 255.0f;
      
      r = (1.0f - a)*dest_r + a*r;
      g = (1.0f - a)*dest_g + a*g;
      b = (1.0f - a)*dest_b + a*b;
      *pixel = (round_f32_to_u32(r * 255.0f) << 16 | round_f32_to_u32(g * 255.0f) << 8 | round_f32_to_u32(b * 255.0f) << 0 | round_f32_to_u32(a * 255.0f) << 24);
      pixel++;
      bmp_pixel++;
    }
  }
}

INTERNAL void renderer_texture(GameBackBuffer *back_buffer, Texture texture, F32 start_x, F32 start_y, F32 width, F32 height) {
  S32 x, y, w, h;
  S32 uv_x, uv_y;
  S32 j, i;
  U32 *pixel;
  U32 *bmp_pixel;
  F32 mapped_range_x, mapped_range_y;
  F32 r, g, b, a;
  F32 dest_r, dest_g, dest_b;
  
  i = j = 0;
  mapped_range_x = mapped_range_y = 0;
  x = round_f32_to_s32(start_x);
  y = round_f32_to_s32(start_y);
  w = round_f32_to_s32(width);
  h = round_f32_to_s32(height);
  for (i = y; (i < (y + h)) && (i < back_buffer->height); ++i) {
    pixel = (CAST(U32 *) back_buffer->memory) + (i * back_buffer->width) + x;
    
    mapped_range_x = map_f32_into_range(CAST(F32)j, start_x, start_x+width);
    mapped_range_y = map_f32_into_range(CAST(F32)i, start_y, start_y+height);
    uv_x = truncate_f32(texture.width * mapped_range_x);
    uv_y = truncate_f32(texture.height * mapped_range_y);
    /* bmp_pixel = CAST(U32 *)texture.data + (uv_y * texture.width) + uv_x; */ /* default y bottom-left */
    bmp_pixel = CAST(U32 *) texture.data + ((texture.width * texture.height) - texture.width * (uv_y + 1)); /* y top-left */
    for (j = x; (j < (x + w)) && (j < back_buffer->width); ++j) {
      mapped_range_x = map_f32_into_range(CAST(F32)j, start_x, start_x+width);
      uv_x = truncate_f32(texture.width * mapped_range_x);
      /* bmp_pixel = CAST(U32 *)texture.data + (uv_y * texture.width) + uv_x; */ /* default y bottom-left */
      bmp_pixel = CAST(U32 *) texture.data + ((texture.width * texture.height) - texture.width * (uv_y + 1)) + uv_x; /* y top-left */
      
      r = (*bmp_pixel >> 16 & 0xFF) / 255.0f;
      g = (*bmp_pixel >> 8 & 0xFF) / 255.0f;
      b = (*bmp_pixel >> 0 & 0xFF) / 255.0f;
      a = (*bmp_pixel >> 24 & 0xFF) / 255.0f;
      
      dest_r = (*pixel >> 16 & 0xFF) / 255.0f;
      dest_g = (*pixel >> 8 & 0xFF) / 255.0f;
      dest_b = (*pixel >> 0 & 0xFF) / 255.0f;
      
      r = (1.0f - a)*dest_r + a*r;
      g = (1.0f - a)*dest_g + a*g;
      b = (1.0f - a)*dest_b + a*b;
      *pixel = (round_f32_to_u32(r * 255.0f) << 16 | round_f32_to_u32(g * 255.0f) << 8 | round_f32_to_u32(b * 255.0f) << 0 | round_f32_to_u32(a * 255.0f) << 24);
      pixel++;
    }
  }
}

/* TODO: Lookup table */
INTERNAL S32 bitmap_font_get_character_offset(U16 c, S32 glyph_width) {
  S32 offset, unknown_char_offset, lowercase_alphabet_start_offset, uppercase_alphabet_start_offset, numbers_start_offset, symbols_start_offset;
  
  unknown_char_offset = 1; /* past the 'unknown' character */
  lowercase_alphabet_start_offset = unknown_char_offset;
  uppercase_alphabet_start_offset = unknown_char_offset + 26;
  numbers_start_offset = uppercase_alphabet_start_offset + 26;
  symbols_start_offset = numbers_start_offset + 10;
  if (c >= 'a' && c <= 'z') {
    offset = (c - 'a') + lowercase_alphabet_start_offset;
  } else if (c >= 'A' && c <= 'Z') {
    offset = (c - 'A') + uppercase_alphabet_start_offset;
  } else if (c >= '0' && c <= '9') {
    offset = (c - '0') + numbers_start_offset;
  } else {
    offset = symbols_start_offset;
    switch (c) {
      case ' ': {
        offset += 64;
      } break;
      
      case '-': {
        offset += 0;
      } break;
      
      case '=': {
        offset += 1;
      } break;
      
      case '(': {
        offset += 2;
      } break;
      case ')': {
        offset += 3;
      } break;
      
      case '{': {
        offset += 4;
      } break;
      case '}': {
        offset += 5;
      } break;
      
      case '[': {
        offset += 6;
      } break;
      case ']': {
        offset += 7;
      } break;
      
      /*
      case '´': {
        offset = 8;
      } break;
*/
      
      case '~': {
        offset += 9;
      } break;
      
      case '<': {
        offset += 10;
      } break;
      case '>': {
        offset += 11;
      } break;
      
      case '/': {
        offset += 12;
      } break;
      
      case '?': {
        offset += 13;
      } break;
      
      case '.': {
        offset += 14;
      } break;
      
      case ',': {
        offset += 15;
      } break;
      
      case '\'': {
        offset += 16;
      } break;
      case '"': {
        offset += 17;
      } break;
      
      case '!': {
        offset += 18;
      } break;
      
      case '@': {
        offset += 19;
      } break;
      
      case '#': {
        offset += 20;
      } break;
      
      case '$': {
        offset += 21;
      } break;
      
      case '%': {
        offset += 22;
      } break;
      
      /*
      case '¨': {
        offset += 23;
      } break;
*/
      
      case '&': {
        offset += 24;
      } break;
      
      case '*': {
        offset += 25;
      } break;
      
      case '|': {
        offset += 26;
      } break;
      
      case '\\': {
        offset += 27;
      } break;
      
      /*
...
      çÇàèìòùáéíóúÀÈÌÒÙÁÉÍÓÚãõÃÕâêôÂÊÔ
...
*/
      
      /*
      case '`': {
        offset += 60;
      } break;
*/
      
      case ':': {
        offset += 61;
      } break;
      
      case ';': {
        offset += 62;
      } break;
      
      case '_': {
        offset += 63;
      } break;
      
      default: {
        offset = 0;
      }
    }
  }
  
  return (glyph_width * offset);
}

INTERNAL void renderer_text(GameBackBuffer *back_buffer, GameBitmapFont *bmp_font, GameColor color, F32 x, F32 y, S8 *text) {
  S32 back_buffer_xpos, back_buffer_ypos;
  S32 texture_xpos, texture_ypos;
  U32 *back_buffer_pixel, *texture_pixel;
  F32 r, g, b, a, dest_r, dest_g, dest_b;
  S32 c;
  S32 i, j;
  
  back_buffer_xpos = round_f32_to_s32(x);
  back_buffer_ypos = round_f32_to_s32(y);
  /* 'text' must be null terminated - just for now, later I'll write a string library */
  while(c = *text++) {
    /* calculate character offset based on the bitmap font */
    texture_ypos = 0;
    texture_xpos = bitmap_font_get_character_offset(c, bmp_font->glyph_width);
    
    if ( (back_buffer_xpos + bmp_font->glyph_width < back_buffer->width) && (back_buffer_ypos + bmp_font->glyph_height < back_buffer->height) ) {
      for (i = 0; i < bmp_font->glyph_height; ++i) {
        back_buffer_pixel = CAST(U32 *)back_buffer->memory + ( (back_buffer_ypos + i) * back_buffer->width) + back_buffer_xpos;
        texture_pixel = CAST(U32 *)bmp_font->bmp.data + ((bmp_font->bmp.width * bmp_font->bmp.height) - bmp_font->bmp.width * (i + 1) + texture_xpos); /* 0,0 is the 'unknown' char - y top-left */
        for (j = 0; j < bmp_font->glyph_width; ++j) {
          r = (*texture_pixel >> 16 & 0xFF) / 255.0f;
          g = (*texture_pixel >> 8 & 0xFF) / 255.0f;
          b = (*texture_pixel >> 0 & 0xFF) / 255.0f;
          a = (*texture_pixel >> 24 & 0xFF) / 255.0f;
          
          if (r == 1.0f && g == 0.0f && b == 1.0f) { /* default debug pink */
            r = color.r;
            g = color.g;
            b = color.b;
          }
          
          dest_r = (*back_buffer_pixel >> 16 & 0xFF) / 255.0f;
          dest_g = (*back_buffer_pixel >> 8 & 0xFF) / 255.0f;
          dest_b = (*back_buffer_pixel >> 0 & 0xFF) / 255.0f;
          
          r = (1.0f - a)*dest_r + a*r;
          g = (1.0f - a)*dest_g + a*g;
          b = (1.0f - a)*dest_b + a*b;
          *back_buffer_pixel = (round_f32_to_u32(r * 255.0f) << 16 | round_f32_to_u32(g * 255.0f) << 8 | round_f32_to_u32(b * 255.0f) << 0 | round_f32_to_u32(a * 255.0f) << 24);
          texture_pixel++;
          back_buffer_pixel++;
        }
      }
      back_buffer_xpos += bmp_font->glyph_width;
    }
  }
}

EXTERN_CLOSE /* } */

#endif /* PONG_RENDERER_H */
