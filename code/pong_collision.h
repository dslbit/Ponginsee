#ifndef PONG_COLLISION_H
#define PONG_COLLISION_H

EXTERN_OPEN /* extern "C" { */

INTERNAL INLINE B32 collision_aabb_vs_aabb(V2 a_origin, F32 a_width, F32 a_height, V2 b_origin, F32 b_width, F32 b_height) {
  B32 is_colliding;
  F32 a_min_x, a_min_y, a_max_x, a_max_y;
  F32 b_min_x, b_min_y, b_max_x, b_max_y;
  
  a_min_x = a_origin.x - a_width/2.0f;
  a_max_x = a_origin.x + a_width/2.0f;
  a_min_y = a_origin.y - a_height/2.0f;
  a_max_y = a_origin.y + a_height/2.0f;
  b_min_x = b_origin.x - b_width/2.0f;
  b_max_x = b_origin.x + b_width/2.0f;
  b_min_y = b_origin.y - b_height/2.0f;
  b_max_y = b_origin.y + b_height/2.0f;
  if (a_min_x <= b_max_x && a_max_x >= b_min_x &&
      a_min_y <= b_max_y && a_max_y >= b_min_y) {
    is_colliding = TRUE;
  }
  return is_colliding;
}

EXTERN_CLOSE /* } */

#endif /* PONG_COLLISION_H */
