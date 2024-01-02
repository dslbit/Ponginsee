#ifndef PONG_MEMORY_H
#define PONG_MEMORY_H

EXTERN_OPEN /* extern "C" { */

/*
-* NOTE: If needed, implement memory arena separation, push struct and push
-* array macros for permanent storage
*/

typedef struct GameMemory GameMemory;
struct GameMemory {
  U64 max_size; /* in bytes */
  U64 current_size; /* in bytes */
  void *address; /* base address */
};

INTERNAL INLINE void *game_memory_push(GameMemory *mem, U64 push_size) {
  void *result;
  
  if (mem->max_size - mem->current_size > push_size) {
    result = CAST(void *) (CAST(U8 *) mem->address + mem->current_size);
    mem->current_size += push_size;
  } else {
    ASSERT(0, L"Game needs more memory!");
  }
  return result;
}

EXTERN_CLOSE /* } */

#endif //PONG_MEMORY_H
