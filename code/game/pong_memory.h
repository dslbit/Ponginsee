#ifndef PONG_MEMORY_H
#define PONG_MEMORY_H

EXTERN_OPEN /* extern "C" { */

/*
-* NOTE: If needed, implement memory arena separation, push struct and push
-* array macros for permanent storage.
-*
-* I could separate the GameMemory into permanent_storage, and a temporary
-* or transient storage. The temporary_storage could be cleared every level
-* or something like that, it depends on the game.
*/

typedef struct GameMemory GameMemory;
struct GameMemory {
  U64 max_size; /* in bytes */
  U64 current_size; /* in bytes */
  void *address; /* base address */
  
  PlatformFreeEntireFileFuncType *platform_free_entire_file;
  PlatformReadEntireFileFuncType *platform_read_entire_file;
  PlatformWriteEntireFileFuncType *platform_write_entire_file;
};

INTERNAL INLINE void *game_memory_push(GameMemory *memory, U64 push_size) {
  void *result;
  
  if (memory->max_size - memory->current_size > push_size) {
    result = CAST(void *) (CAST(U8 *) memory->address + memory->current_size);
    memory->current_size += push_size;
  } else {
    ASSERT(0, L"Game needs more memory!");
  }
  return result;
}

EXTERN_CLOSE /* } */

#endif /* PONG_MEMORY_H */
