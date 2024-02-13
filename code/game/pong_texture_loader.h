#ifndef PONG_TEXTURE_LOADER_H
#define PONG_TEXTURE_LOADER_H

#define DEFAULT_TEXTURE_BYTES_PER_PIXEL (4)

EXTERN_OPEN /* extern "C" { */

typedef struct Texture Texture;
struct Texture {
  S32 width;
  S32 height;
  void *data;
};


EXTERN_CLOSE /* } */

#endif /* PONG_TEXTURE_LOADER_H */
