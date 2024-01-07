#ifndef PONG_COLOR_H
#define PONG_COLOR_H

EXTERN_OPEN /* extern "C" { */

/* NOTE: Maybe use a V4? */
typedef struct GameColor GameColor;
struct GameColor {
  F32 r;
  F32 g;
  F32 b;
  F32 a;
};

INTERNAL INLINE GameColor color_create(F32 r, F32 g, F32 b, F32 a) {
  GameColor result;
  
  result.r = r;
  result.g = g;
  result.b = b;
  result.a = a;
  return result;
}

/* NOTE: Format RR GG BB AA */
INTERNAL INLINE GameColor color_create_from_hex(U32 color) {
  GameColor result;
  
  result.r = CAST(F32) (0xFF & (color >> 24)) / 255.0f;
  result.g = CAST(F32) (0xFF & (color >> 16)) / 255.0f;
  result.b = CAST(F32) (0xFF & (color >> 8)) / 255.0f;
  result.a = CAST(F32) (0xFF & (color >> 0)) / 255.0f;
  return result;
}

INTERNAL INLINE GameColor color_create_from_rgba(U8 r, U8 g, U8 b, U8 a) {
  GameColor result;
  
  result.r = CAST(F32) r / 255.0f;
  result.g = CAST(F32) g / 255.0f;
  result.b = CAST(F32) b / 255.0f;
  result.a = CAST(F32) a / 255.0f;
  return result;
}

EXTERN_CLOSE /* } */

#endif /* PONG_COLOR_H */
