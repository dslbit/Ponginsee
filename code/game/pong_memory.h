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
  U64 permanent_max_size; /* in bytes */
  U64 permanent_current_size; /* in bytes */
  void *permanent_address; /* base address */
  
  U64 transient_max_size;
  U64 transient_current_size;
  void *transient_address;
  
  PlatformFreeEntireFileFuncType *platform_free_entire_file;
  PlatformReadEntireFileFuncType *platform_read_entire_file;
  PlatformWriteEntireFileFuncType *platform_write_entire_file;
};

INTERNAL void *game_memory_push_permanent(GameMemory *memory, U64 push_size) {
  void *result;
  
  if (memory->permanent_max_size - memory->permanent_current_size > push_size) {
    result = CAST(void *) (CAST(U8 *) memory->permanent_address + memory->permanent_current_size);
    memory->permanent_current_size += push_size;
  } else {
    ASSERT(0, L"Game needs more memory!");
  }
  return result;
}

INTERNAL void *game_memory_push_transient(GameMemory *memory, U64 push_size) {
  void *result;
  
  if (memory->transient_max_size - memory->transient_current_size > push_size) {
    result = CAST(void *) (CAST(U8 *) memory->transient_address + memory->transient_current_size);
    memory->transient_current_size += push_size;
  } else {
    ASSERT(0, L"Game needs more memory!");
  }
  return result;
}

INTERNAL void game_memory_clear_transient(GameMemory *memory) {
  debug_zero_array(memory->transient_address, memory->transient_max_size);
  memory->transient_current_size = 0;
}

EXTERN_CLOSE /* } */

#endif /* PONG_MEMORY_H */
