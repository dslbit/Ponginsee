#ifndef PONG_MATH_H
#define PONG_MATH_H

#define SQUARE(_x) ((_x) * (_x))

EXTERN_OPEN /* extern "C" { */

/*
-* Vector types
*/

typedef struct V2 V2;
struct V2 {
  F32 x;
  F32 y;
};

/*
-* Matrix types
*/

typedef struct M2 M2;
struct M2 {
  union {
    F32 e[2][2];
    struct {
      F32 _00;
      F32 _01;
      F32 _10;
      F32 _11;
    };
  };
};

/*
-* Shape types
*/

typedef struct Rect2 Rect2;
struct Rect2 {
  V2 p[4];
};


/*
-* Scalars
*/

INTERNAL INLINE U32 truncate_u64_to_u32(U64 value) {
  U32 result;
  
  ASSERT(value <= UINT32_MAX, L"It's not safe to truncate this value!");
  result = CAST(U32) value;
  return result;
}

INTERNAL INLINE S32 truncate_f32(F32 value) {
  S32 result;
  
  result = CAST(S32) value;
  return result;
}

/* NOTE: Not fully tested */
INTERNAL INLINE F32 round_f32(F32 value) {
  F32 result;
  
  if (value > 0) {
    result = CAST(F32) floor((value + 0.5f));
  } else {
    result = CAST(F32) floor((value + -0.5f));
  }
  
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

INTERNAL INLINE F32 sin_f32(F32 a) {
  F32 result;
  
  result = sinf(a);
  return result;
}

INTERNAL INLINE F32 cos_f32(F32 a) {
  F32 result;
  
  result = cosf(a);
  return result;
}

INTERNAL INLINE F32 deg_to_rad(F32 deg) {
  F32 result;
  
  result = deg * (PI / 180.0f);
  return result;
}

INTERNAL INLINE F32 rad_to_deg(F32 rad) {
  F32 result;
  
  result = rad * (180.0f / PI);
  return result;
}

INTERNAL INLINE F32 map_f32_into_range(F32 value, F32 min, F32 max) { /* maps value [0 ... 1] */
  F32 result;
  
  result = CLAMP((value - min) / (max - min), 0, 1);
  return result;
}

/*
-* 2D Vectors
*/

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

INTERNAL INLINE F32 v2_mag(V2 a) {
  F32 result;
  
  result = sqrtf(SQUARE(a.x) + SQUARE(a.y));
  return result;
}

INTERNAL INLINE F32 v2_mag_squared(V2 a) {
  F32 result;
  
  result = v2_dot(a, a);
  return result;
}

INTERNAL INLINE V2 v2_norm(V2 a) {
  F32 mag;
  V2 result = {0};
  
  mag = v2_mag(a);
  if (mag != 0) {
    result.x = a.x / mag;
    result.y = a.y / mag;
  }
  return result;
}

INTERNAL INLINE F32 v2_angle(V2 a) {
  F32 result;
  
  result = atan2f(a.y, a.x);
  return result;
}

/*
-* 2D Matrices
*/

INTERNAL INLINE M2 m2_create(F32 _00, F32 _01, F32 _10, F32 _11) {
  M2 result;
  
  result._00 = _00;
  result._01 = _01;
  result._10 = _10;
  result._11 = _11;
  return result;
}

INTERNAL INLINE V2 m2_mul_v2(M2 m, V2 v) {
  V2 result;
  
  result = v2_create( ((v.x * m._00) + (v.y * m._01)), ((v.x * m._10) + (v.y * m._11)) );
  return result;
}

/*
-* Polygon shapes
*/

INTERNAL INLINE Rect2 rect2_create(V2 min, V2 max) {
  Rect2 result;
  
  result.p[0] = min;
  result.p[1] = v2_create(max.x, min.y);
  result.p[2] = max;
  result.p[3] = v2_create(min.x, max.y);
  return result;
}
EXTERN_CLOSE /* } */
#endif /* PONG_MATH_H */
