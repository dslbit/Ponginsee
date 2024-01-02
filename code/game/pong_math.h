#ifndef PONG_MATH_H
#define PONG_MATH_H

#define SQUARE(_x) ((_x) * (_x))

EXTERN_OPEN /* extern "C" { */

/* NOTE: Round towards zero - Not good for negative values */
INTERNAL INLINE F32 round_f32(F32 value) {
  F32 result;
  
  result = CAST(F32) (CAST(S32) (value + 0.5f));
  return result;
}

/* NOTE: Round towards zero - Not good for negative values */
INTERNAL INLINE S32 round_f32_to_s32(F32 value) {
  S32 result;
  
  result = CAST(S32) (value + 0.5f);
  return result;
}

/* NOTE: Round towards zero - Not good for negative values */
INTERNAL INLINE U32 round_f32_to_u32(F32 value) {
  U32 result;
  
  result = CAST(U32) (value + 0.5f);
  return result;
}

typedef struct V2 V2;
struct V2 {
  F32 x;
  F32 y;
};

INTERNAL INLINE V2 v2_create(F32 x, F32 y) {
  V2 result;
  
  result.x = x;
  result.y = y;
  return result;
}

INTERNAL INLINE void v2_zero(V2 *a) {
  a->x = 0;
  a->y = 0;
}

INTERNAL INLINE V2 v2_add(V2 a, V2 b) {
  V2 result;
  
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  return result;
}

INTERNAL INLINE V2 v2_sub(V2 a, V2 b) {
  V2 result;
  
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  return result;
}

INTERNAL INLINE V2 v2_mul(V2 a, F32 value) {
  V2 result;
  
  result.x = a.x * value;
  result.y = a.y * value;
  return result;
}

INTERNAL INLINE V2 v2_div(V2 a, F32 value) {
  V2 result;
  
  if (value != 0) {
    result.x = a.x / value;
    result.y = a.y / value;
  }
  return result;
}

INTERNAL INLINE F32 v2_dot(V2 a, V2 b) {
  F32 result;
  
  result = (a.x * b.x) + (a.y * b.y);
  return result;
}

INTERNAL INLINE F32 v2_mag_squared(V2 a) {
  F32 result;
  
  result = v2_dot(a, a);
  return result;
}

EXTERN_CLOSE /* } */
#endif //PONG_MATH_H
