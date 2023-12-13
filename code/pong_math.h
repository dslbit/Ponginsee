#ifndef PONG_MATH_H
#define PONG_MATH_H

#define SQUARE(_x) ((_x) * (_x))

typedef struct V2 V2;
struct V2 {
  F32 x;
  F32 y;
};

V2 v2_create(F32 x, F32 y) {
  V2 result;
  
  result.x = x;
  result.y = y;
  return result;
}

void v2_zero(V2 *a) {
  a->x = 0;
  a->y = 0;
}

V2 v2_add(V2 a, V2 b) {
  V2 result;
  
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  return result;
}

V2 v2_sub(V2 a, V2 b) {
  V2 result;
  
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  return result;
}

V2 v2_mul(V2 a, F32 value) {
  V2 result;
  
  result.x = a.x * value;
  result.y = a.y * value;
  return result;
}

V2 v2_div(V2 a, F32 value) {
  V2 result;
  
  if (value != 0) {
    result.x = a.x / value;
    result.y = a.y / value;
  }
  return result;
}

F32 v2_dot(V2 a, V2 b) {
  F32 result;
  
  result = (a.x * b.x) + (a.y * b.y);
  return result;
}

F32 v2_mag_squared(V2 a) {
  F32 result;
  
  result = v2_dot(a, a);
  return result;
}

#endif //PONG_MATH_H
