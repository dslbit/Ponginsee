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

/* NOTE: Only accepts ARGB 32-bit '.bmp' files (for now) */
INTERNAL Texture load_bitmap(GameMemory *memory, U16 *file_name) {
  Texture result = {0};
  ReadFileResult bitmap = {0};
  U32 pixel_array_offset;
  
  bitmap = memory->platform_read_entire_file(file_name);
  if (bitmap.data) {
    U8 *base;
    
    base = bitmap.data;
    /* bitmap header - file identification */
    {
      U8 bmp_identification[2];
      
      bmp_identification[0] = *(base + 0);
      bmp_identification[1] = *(base + 1);
      if (bmp_identification[0] != 'B' || bmp_identification[1] != 'M') {
        ASSERT(0, L"File isn't a '.bmp' file format!");
      }
    }
    
    base = bitmap.data;
    base += 2;
    /* bitmap header - file size in bytes for debug purposes */
    {
      U32 size;
      
      size = *(CAST(U32 *) base);
      size = size;
    }
    
    base = bitmap.data;
    base += 10;
    /* bitmap header - pixel array data offset */
    {
      pixel_array_offset = *(CAST(U32 *) base);
    }
    
    base = bitmap.data;
    base += 14;
    /* bitmap header image info */
    {
      U32 header_size;
      S32 width;
      S32 height;
      U32 size; /* size of the bitmap in bytes */
      
      header_size = *(CAST(U32 *) base);
      base += 4;
      width = *(CAST(S32 *) base);
      result.width = width;
      
      base += 4;
      height = *(CAST(S32 *) base);
      result.height = height;
      
      base += 12;
      size = *(CAST(U32*) base);
      if (size != (width * height * DEFAULT_TEXTURE_BYTES_PER_PIXEL)) {
        ASSERT(0, L"Bitmap file pixel color information is invalid!");
      }
    }
    
    /* allocate memory for the texture, and copy pixel data */
    {
      U32 texture_size; /* in bytes */
      S32 i;
      U32 *bmp_pixels;
      U32 *texture_pixels;
      
      texture_size = result.width * result.height * DEFAULT_TEXTURE_BYTES_PER_PIXEL;
      result.data = game_memory_push(memory, texture_size);
      if (!result.data) {
        ASSERT(0, L"Game couldn't allocate memory for the texture!");
      }
      base = bitmap.data;
      base += pixel_array_offset; /* start at the end and copy pixels data backwords - flip bitmap y-direction */
      bmp_pixels = CAST(U32 *) base;
      texture_pixels = CAST(U32 *) result.data;
      /* hacked memcpy */
      for (i = 0; i < (result.width * result.height); ++i) {
        *texture_pixels = *bmp_pixels;
        texture_pixels++;
        bmp_pixels++;
      }
    }
  } else {
    ASSERT(0, L"Couldn't load the bitmap!");
  }
  return result;
}

EXTERN_CLOSE /* } */

#endif /* PONG_TEXTURE_LOADER_H */
