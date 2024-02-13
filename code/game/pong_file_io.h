#ifndef PONG_FILE_IO_H
#define PONG_FILE_IO_H

EXTERN_OPEN /* extern "C" { */

typedef struct ReadFileResult ReadFileResult;
struct ReadFileResult {
  U32 data_size;
  void *data;
};

typedef void PlatformFreeEntireFileFuncType(void *address);
typedef ReadFileResult PlatformReadEntireFileFuncType(U16 *file_name);
typedef B32 PlatformWriteEntireFileFuncType(U16 *file_name, U32 data_size, void *data);

EXTERN_CLOSE /* } */

#endif //PONG_FILE_IO_H
