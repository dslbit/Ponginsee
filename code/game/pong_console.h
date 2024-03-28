#ifndef PONG_CONSOLE_H
#define PONG_CONSOLE_H

/* IMPORTANT: Temporary; Replace! */
#include "stdio.h" /* used for snprintf() */

#define GAME_CONSOLE_BUFFER_MAX_STACK_SIZE (128)
#define GAME_CONSOLE_INPUT_MAX_LENGTH (64)

EXTERN_OPEN

typedef struct GameConsoleState GameConsoleState;
struct GameConsoleState {
  B32 is_on;
  GameColor color_bg;
  GameColor color_border;
  GameColor color_text;
  GameColor color_input;
  S8 buffer[GAME_CONSOLE_BUFFER_MAX_STACK_SIZE][GAME_CONSOLE_INPUT_MAX_LENGTH + 1]; /* display buffer */
  S32 buffer_last_index;
  S8 input[GAME_CONSOLE_INPUT_MAX_LENGTH + 1]; /* input buffer */
  S32 input_last_index;
};

/* TODO: ring buffer */
INTERNAL void console_write(GameConsoleState *console, S8 *str) {
  if (console->buffer_last_index <= (ARRAY_COUNT(console->buffer) - 1)) {
    snprintf(console->buffer[console->buffer_last_index], ARRAY_COUNT(console->buffer[console->buffer_last_index]), "%s", str);
    ++console->buffer_last_index;
  }
}

/* TODO: Error check & ring buffer? */
INTERNAL void console_move_input_to_display_buffer(GameConsoleState *console) {
  snprintf(console->buffer[console->buffer_last_index], ARRAY_COUNT(console->buffer[console->buffer_last_index]), "%s", console->input); /* TODO: replace for custom func */
  ++console->buffer_last_index;
  debug_zero_array(console->input, ARRAY_COUNT(console->input));
  console->input_last_index = 0;
}

EXTERN_CLOSE

#endif /* PONG_CONSOLE_H */
