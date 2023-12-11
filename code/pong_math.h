#ifndef PONG_MATH_H
#define PONG_MATH_H

typedef struct V2 V2;
struct V2 {
  F32 x;
  F32 y;
};

V2 v2_create(F32 x, F32 y) {
  V2 result = {0};
  
  result.x = x;
  result.y = y;
  return result;
}

void v2_zero(V2 *a) {
  a->x = 0;
  a->y = 0;
}

V2 v2_add(V2 a, V2 b) {
  V2 result = {0};
  
  result.x = a.x + b.x;
  result.y = a.y + b.y;
  return result;
}

V2 v2_sub(V2 a, V2 b) {
  V2 result = {0};
  
  result.x = a.x - b.x;
  result.y = a.y - b.y;
  return result;
}

V2 v2_mul(V2 a, F32 value) {
  V2 result = {0};
  
  result.x = a.x * value;
  result.y = a.y * value;
  return result;
}

V2 v2_div(V2 a, F32 value) {
  V2 result = {0};
  
  if (value != 0) {
    result.x = a.x / value;
    result.y = a.y / value;
  }
  return result;
}

#endif //PONG_MATH_H
