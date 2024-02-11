#ifndef PONG_FILE_IO_H
#define PONG_FILE_IO_H

EXTERN_OPEN /* extern "C" { */

typedef struct ReadFileResult ReadFileResult;
struct ReadFileResult {
  U64 data_size;
  void *data;
};

#define PLATFORM_FREE_ENTIRE_FILE_PROTOTYPE(_name) void _name(void *address)
typedef PLATFORM_FREE_ENTIRE_FILE_PROTOTYPE(PlatformFreeEntireFileType);

#define PLATFORM_READ_ENTIRE_FILE_PROTOTYPE(_name) ReadFileResult _name(U16 *file_name) /* path is relative to the executable root folder */
typedef PLATFORM_READ_ENTIRE_FILE_PROTOTYPE(PlatformReadEntireFileType);

#define PLATFORM_WRITE_ENTIRE_FILE_PROTOTYPE(_name) B32 _name(U16 *file_name, U32 data_size, void *data)
typedef PLATFORM_WRITE_ENTIRE_FILE_PROTOTYPE(PlatformWriteEntireFileType);

EXTERN_CLOSE /* } */

#endif //PONG_FILE_IO_H
