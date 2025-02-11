/*
-* ROBUST PASS:
-*  |_-> Debug simplified console (with support for integers & floating-point numbers)
-*   |_-> (for the future) What about tunning & query variables through the console? e.g.: turn_on_particles, turn_off_trails...
-*   |_-> (for the future) Commands like load_level,  etc.
-*   |_-> (for the future) To every system created, output something to the engine console using a certain debug level
-*   |_-> (for the future) Find a way to display the current level in the debug infos
-*  |_-> Texture rendering with ( ) rotation and (X) relative coordinate system (UV) for scaled bitmaps
-*  
-*  |_-> NOTE: Should I pass the origin to the renderer?
-*  |_-> +1 effect for points when player hit the ball (juice)
-*  
-*  |_-> Pull out the entity vs arena code? Maybe return a v2 -1 to 1 range to
-*  identify where entity was before going out of bounds
-*  |_-> (for the future) Save progress? - IDK how crazy this will get, so do it only if it's
-*  needed
-*  
-*  |_-> Figure out text rendering using (X) Bitmaps & () TrueType
-*  
-*  |_-> Robust debug info and engine console.
-*  
-*  |_-> Menu screen (Play, Scoreboard (stores personal records), Settings
-*  (Resolution, Audio, Controls), Quit)
-*  |_-> Pause menu (Main menu, Quit Game)
#---
-* TODO LIST:
-*  |_-> Figure out the sound engine
-*  |_-> Platform-independent: sound output
-*
#---
-* NOTE:
-*  |_-> When I press the pause button when go fullscreen on/off, the 'back buffer' is not rendered.
-* 
*/

#include "pong_base.h"
#include "pong_file_io.h"
#include "pong_math.h"
#include "pong_color.h"
#include "pong_memory.h"
#include "pong_texture_loader.h"
#include "pong_particle.h"
#include "pong_entity.h"
#include "pong_collision.h"
#include "pong_level.h"
#include "pong_console.h"
#include "pong_platform.h"
#include "pong_renderer.h"

/* NOTE: What if other levels have the same movement code? */
INTERNAL void level_menu(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory);
INTERNAL void level_test(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory);
INTERNAL void level_classic(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory);
INTERNAL void level_end(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory);

void game_update_and_render(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory) {
  GameState *state; /* TODO: Do the same for other global game states (debug, console...) */
  
  ASSERT(sizeof(GameState) < memory->permanent_max_size, L"NOT ENOUGH MEMORY!");
  state = CAST(GameState *) memory->permanent_address;
  if (!state->is_initialized) {
    state->is_initialized = TRUE;
    
    state->bmp_font_default.glyph_width = 8;
    state->bmp_font_default.glyph_height = 16;
    state->bmp_font_default.bmp = load_bitmap(memory, GAME_DEFAULT_DATA_RELATIVE_PATH GAME_BMP_FONT_DEFAULT);
    state->text_default_color = color_create_from_hex(0xefd081ff);
    state->text_color_green = color_create_from_hex(0x3ec54bff);
    state->text_color_red = color_create_from_hex(0xf5464cff);
    state->text_color_black = color_create_from_hex(0x1f1723be);
    
    state->game_console_state.is_on = FALSE;
    state->game_console_state.color_bg = color_create_from_hex(0x1f1723be);
    state->game_console_state.color_border = color_create_from_hex(0xc6c6c67f);
    state->game_console_state.color_text = color_create_from_hex(0xb4e656ff);
    state->game_console_state.color_input = color_create_from_hex(0x3ec54bff);
    
    state->game_debug_state.is_on = FALSE;
    state->game_debug_state.dt = 0.016666f;
    state->game_debug_state.is_particles_on = TRUE;
    state->game_debug_state.is_trails_on = TRUE;
    state->game_debug_state.is_ddp_effects_on = TRUE;
    state->background_color = color_create_from_hex(0x1f1723ff);
    state->background_color_paused = color_create_from_hex(0xefd08180);
    /* NOTE: Maybe this should be moved to the level update function
        initialization place */
    state->game_level.id = LEVEL_ID_MENU;
    state->game_level.is_initialized = FALSE;
    state->game_level.is_running = FALSE;
    state->game_level.time_elapsed = 0.0f;
    state->game_level.time_max = 5.0f;
    state->game_level.min_bounding_rect_x = 0;
    state->game_level.max_bounding_rect_x = back_buffer->width;
    state->game_level.min_bounding_rect_y = 0;
    state->game_level.max_bounding_rect_y = back_buffer->height;
    
    state->player = entity_create(ENTITY_TYPE_PLAYER);
    state->player.color = color_create_from_hex(0x4656a5ff);
    state->player.player_data.score_accumulation = 0.0f;
    input->player1.enabled = TRUE;
    
    state->opponent = entity_create(ENTITY_TYPE_PLAYER);
    state->opponent.color = color_create_from_hex(0xf5464cff);
    state->opponent.player_data.score_accumulation = 0.0f;
    
    state->ball = entity_create(ENTITY_TYPE_BLANK);
    state->ball.color = color_create_from_hex(0x3ec54b60);
    state->ball.width = 7;
    state->ball.height = 7;
    state->ball.ball_data.size_multiplier = 1;
  }
  
  /* NOTE: Engine debug mode - Update */
  {
    GameDebugState *debug;
    
    debug = &state->game_debug_state;
    /* debug mode toggle */
    if (input->player1.f3.released) {
      debug->is_on = !debug->is_on;
    }
    
    /* dt change - TODO: change names, it's pageup/pagedown */
    if (debug->is_on) {
      if (input->player1.minus.released) {
        debug->dt += 0.0023334f;
      }
      if (input->player1.plus.released) {
        debug->dt -= 0.0023334f;
        if (debug->dt < 0) {
          debug->dt = 0;
        }
      }
    }
    
    /* set debug state if it's on */
    debug->dt_original = input->dt;
    if (debug->is_on) {
      input->dt = debug->dt;
    }
    
    /* update accumulated_dt */
    debug->accumulated_dt += debug->dt_original;
    if (debug->accumulated_dt < 0) {
      debug->accumulated_dt = 0; /* account for almost impossible overflow */
    }
  }
  
  /* Engine console - Update - TODO: buffer view/height indicator and controller to change the display buffer text 'position' */
  {
    GameConsoleState *console;
    GameDebugState *debug;
    LOCAL F32 counter; /* NOTE: temp, @cleanup */
    B32 is_text_stream_empty;
    
    console = &state->game_console_state;
    debug = &state->game_debug_state;
    if (input->player1.f9.released) { /* TODO: Close the engine console with 'esc' too */
      console->is_on = !console->is_on;
      input->player1.enabled = !input->player1.enabled; /* block player game input, keys can be captured via the input buffer and threated individually */
    }
    
    /* TODO: resize buffer size for max msg storage */
    is_text_stream_empty = (input->text_stream.last_index == 0);
    if (console->is_on && !is_text_stream_empty) {
      S32 i;
      
      if (console->input_last_index + input->text_stream.last_index < GAME_CONSOLE_INPUT_MAX_LENGTH) {
        S32 sum;
        
        sum = 0;
        for (i = 0; i < input->text_stream.last_index; ++i) {
          if ( input->text_stream.stream[i] == 0x08 ) { /* ASCII backspace - erase input */ 
            if (console->input_last_index > 0) {
              console->input[console->input_last_index - 1] = 0; /* TODO: It seems that the bug is near here - when I try to show the buffer after filling it and doing 1 backspace, it bugs out */
              --console->input_last_index;
            } else {
              *input->text_stream.stream = '\0';
            }
          } else if ( input->text_stream.stream[i] == 0x0D) { /* ASCII return - moves input to the display buffer and clear old input */
            console_move_input_to_display_buffer(console);
          }
          
          /* if is not invalid, copy the character */
          {
            if ( (input->text_stream.stream[i] >= 32) && (input->text_stream.stream[i] <= 126) ) {
              console->input[console->input_last_index] = input->text_stream.stream[i];
              ++console->input_last_index;
            }
            
          }
        }
        
#if 0
        strcat(console->input, input->text_stream.stream);
        console->input_last_index += sum;
#endif
        
      } else {
        for (i = 0; i < input->text_stream.last_index; ++i) {
          if ( input->text_stream.stream[i] == 0x08 ) { /* ASCII backspace - (erase input) when input is full*/ 
            console->input[console->input_last_index - 1] = 0;
            --console->input_last_index;
          } else if (input->text_stream.stream[i] == 0x0D) { /* ASCII return - (moves input to the display buffer) when input is full */
            console_move_input_to_display_buffer(console);
          }
        }
        
      }
    }
    
#if 0
    counter += input->dt;
    if (counter > 2.0f) {
      counter = 0.0f;
      console_write(console, "Console buffer update test!");
      /* push msg to the end of console buffer  */
      if (console->buffer_last_index != (ARRAY_COUNT(console->buffer) - 1)) {
        snprintf(console->buffer[console->buffer_last_index], ARRAY_COUNT(console->buffer[console->buffer_last_index]), "%f", debug->accumulated_dt);
        ++console->buffer_last_index;
      }
    }
#endif
  }
  
  /* NOTE: Primitve level selection & pause action */
  {
    if (input->player1.enabled) {
      /* pause state */
      if(input->player1.start.released && (state->game_level.id != LEVEL_ID_MENU)) {
        if (state->is_paused) {
          state->is_showing_paused_screen = FALSE;
        }
        state->is_paused = !state->is_paused;
      }
      
      /* return to menu state */
      if (input->player1.back.released && (state->game_level.id != LEVEL_ID_MENU)) {
        if (state->is_paused) {
          state->is_showing_paused_screen = FALSE;
          state->is_paused = !state->is_paused;
        }
        state->game_level.is_initialized = FALSE;
        state->game_level.is_running = FALSE;
        state->game_level.id = LEVEL_ID_MENU;
        game_memory_clear_transient(memory);
      }
      
      /* press start to play state */
      if (input->player1.start.released && (state->game_level.id == LEVEL_ID_MENU)) {
        state->game_level.is_initialized = FALSE;
        state->game_level.is_running = FALSE;
        state->game_level.id = LEVEL_ID_CLASSIC;
      }
      
      /* TODO: assert to only dev build */
      /* Load test level shortcut - Collision test */
      if (input->player1.aux0.released) {
        state->game_level.is_initialized = FALSE;
        state->game_level.is_running = FALSE;
        state->game_level.id = LEVEL_ID_TEST;
        game_memory_clear_transient(memory);
      }
      
      /* Load classic level shortcut */
      if (input->player1.aux1.released) {
        state->game_level.is_initialized = FALSE;
        state->game_level.is_running = FALSE;
        state->game_level.id = LEVEL_ID_CLASSIC;
        game_memory_clear_transient(memory);
      }
    }
  }
  
  if (state->is_paused) {
    if (!state->is_showing_paused_screen) {
      state->is_showing_paused_screen = TRUE;
      
      /* TODO: Better background clear */
      renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, CAST(F32) back_buffer->width, CAST(F32) back_buffer->height, state->background_color_paused);
    }
    return;
  }
  
  /* NOTE: Level update and render */
  switch (state->game_level.id) {
    case LEVEL_ID_MENU: {
      level_menu(back_buffer, input, memory);
    } break;
    
    case LEVEL_ID_TEST: {
      level_test(back_buffer, input, memory);
    } break;
    
    case LEVEL_ID_CLASSIC: {
      level_classic(back_buffer, input, memory);
    } break;
    
    case LEVEL_ID_END: {
      level_end(back_buffer, input, memory);
    } break;
    
    default: {
      ASSERT(0, L"Invalid game level ID.");
    }
  }
  
  /* NOTE: Engine debug mode drawing over */
  {
    GameDebugState *debug;
    
    debug = &state->game_debug_state;
    if (debug->is_on) {
      
      /* debug rect indication (the red border around the screen) */
      {
        F32 border_size;
        
        border_size = 3;
        renderer_filled_rect(back_buffer, 0+border_size/2.0f, back_buffer->height/2.0f, border_size, CAST(F32) back_buffer->height, color_create_from_rgba(178, 39, 65, 160)); /* left border */
        
        renderer_filled_rect(back_buffer, back_buffer->width - border_size/2.0f, back_buffer->height/2.0f, border_size, CAST(F32) back_buffer->height, color_create_from_rgba(178, 39, 65, 160)); /* right border */
        
        renderer_filled_rect(back_buffer, back_buffer->width/2.0f, border_size/2.0f, CAST(F32) back_buffer->width - border_size*2, border_size, color_create_from_rgba(178, 39, 65, 160)); /* top border */
        
        renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height-border_size/2.0f, CAST(F32) back_buffer->width - border_size*2, border_size, color_create_from_rgba(178, 39, 65, 160)); /* top border */
        
        /* TODO: Draw collision bounds */
      }
      
      {
        /* Debug Info (text) - TODO: instead of this hacky way of aligning the text to the display screen, make a function debug_info_add(s, ...) */
        F32 text_x, text_y, text_yspacing;
        S8 text_buffer[1024] = {0};
        
        text_x = 0.0f + state->bmp_font_default.glyph_width/2.0f;
        text_y = 0.0f + state->bmp_font_default.glyph_height;
        text_yspacing = state->bmp_font_default.glyph_height + state->bmp_font_default.glyph_height/6.0f;
        
        /* NOTE: @Replace snprintf() */
        snprintf(text_buffer, ARRAY_COUNT(text_buffer), "MS (original): %f", state->game_debug_state.dt_original*1000);
        renderer_text(back_buffer, &state->bmp_font_default, state->text_default_color, text_x, text_y, text_buffer);
        text_y += text_yspacing;
        
        snprintf(text_buffer, ARRAY_COUNT(text_buffer), "MS (imposed): %f", state->game_debug_state.dt*1000);
        renderer_text(back_buffer, &state->bmp_font_default, state->text_default_color, text_x, text_y, text_buffer);
        text_y += text_yspacing;
        
        snprintf(text_buffer, ARRAY_COUNT(text_buffer), "Accumulated dt: %f", state->game_debug_state.accumulated_dt);
        renderer_text(back_buffer, &state->bmp_font_default, state->text_default_color, text_x, text_y, text_buffer);
        text_y += text_yspacing;
        
        snprintf(text_buffer, ARRAY_COUNT(text_buffer), "Double Buffer: %i x %i", back_buffer->width, back_buffer->height);
        renderer_text(back_buffer, &state->bmp_font_default, state->text_default_color, text_x, text_y, text_buffer);
        text_y += text_yspacing;
        
        /* Text: 'Game Memory: ...' */
        snprintf(text_buffer, ARRAY_COUNT(text_buffer), "Game Memory (permanent): %lliKB", memory->permanent_current_size/1024);
        renderer_text(back_buffer, &state->bmp_font_default, state->text_default_color, text_x, text_y, text_buffer);
        text_y += text_yspacing;
        
        snprintf(text_buffer, ARRAY_COUNT(text_buffer), "Game Memory (transient): %lliKB", memory->transient_current_size/1024);
        renderer_text(back_buffer, &state->bmp_font_default, state->text_default_color, text_x, text_y, text_buffer);
        text_y += text_yspacing;
        
        /* Maybe do a count for particles/trails/ddps etc. - it almost ready, just need to draw now */
        
        /* Text: 'Particles: ON/OFF' */
        text_x += state->bmp_font_default.glyph_width * 40; /* 40 chars */
        text_y = 0.0f + state->bmp_font_default.glyph_height;
        
        if (state->game_debug_state.is_particles_on) {
          snprintf(text_buffer, ARRAY_COUNT(text_buffer), "Particles: ");
          renderer_text(back_buffer, &state->bmp_font_default, state->text_default_color, text_x, text_y, text_buffer);
          
          snprintf(text_buffer, ARRAY_COUNT(text_buffer), "ON");
          renderer_text(back_buffer, &state->bmp_font_default, state->text_color_green, text_x + ((ARRAY_COUNT("Particles: ") - 1) * state->bmp_font_default.glyph_width), text_y, text_buffer);
          
          snprintf(text_buffer, ARRAY_COUNT(text_buffer), " - %d", state->game_debug_state.count_particles);
          renderer_text(back_buffer, &state->bmp_font_default, state->text_default_color, text_x + ((ARRAY_COUNT("Particles: ON") - 1) * state->bmp_font_default.glyph_width), text_y, text_buffer);
        } else {
          snprintf(text_buffer, ARRAY_COUNT(text_buffer), "OFF");
          renderer_text(back_buffer, &state->bmp_font_default, state->text_color_red, text_x + ((ARRAY_COUNT("Particles: ") - 1) * state->bmp_font_default.glyph_width), text_y, text_buffer);
        }
        text_y += text_yspacing;
        
        /* Text: 'Trails: ON/OFF' */
        if (state->game_debug_state.is_trails_on) {
          snprintf(text_buffer, ARRAY_COUNT(text_buffer), "Trails: ");
          renderer_text(back_buffer, &state->bmp_font_default, state->text_default_color, text_x, text_y, text_buffer);
          
          snprintf(text_buffer, ARRAY_COUNT(text_buffer), "ON");
          renderer_text(back_buffer, &state->bmp_font_default, state->text_color_green, text_x + ((ARRAY_COUNT("Trails: ") - 1) * state->bmp_font_default.glyph_width), text_y, text_buffer);
          
          snprintf(text_buffer, ARRAY_COUNT(text_buffer), " - %d", state->game_debug_state.count_trails);
          renderer_text(back_buffer, &state->bmp_font_default, state->text_default_color, text_x + ((ARRAY_COUNT("TRAILS: ON") - 1) * state->bmp_font_default.glyph_width), text_y, text_buffer);
        } else {
          snprintf(text_buffer, ARRAY_COUNT(text_buffer), "Trails: ");
          renderer_text(back_buffer, &state->bmp_font_default, state->text_default_color, text_x, text_y, text_buffer);
          snprintf(text_buffer, ARRAY_COUNT(text_buffer), "OFF");
          renderer_text(back_buffer, &state->bmp_font_default, state->text_color_red, text_x + ((ARRAY_COUNT("Trails: ") - 1) * state->bmp_font_default.glyph_width), text_y, text_buffer);
        }
        text_y += text_yspacing;
        
        /* Text: 'DDP Effects: ON/OFF' */
        if (state->game_debug_state.is_trails_on) {
          snprintf(text_buffer, ARRAY_COUNT(text_buffer), "DDP Effects: ");
          renderer_text(back_buffer, &state->bmp_font_default, state->text_default_color, text_x, text_y, text_buffer);
          
          snprintf(text_buffer, ARRAY_COUNT(text_buffer), "ON");
          renderer_text(back_buffer, &state->bmp_font_default, state->text_color_green, text_x + ((ARRAY_COUNT("DDP Effects: ") - 1) * state->bmp_font_default.glyph_width), text_y, text_buffer);
          
          snprintf(text_buffer, ARRAY_COUNT(text_buffer), " - %d", state->game_debug_state.count_ddp_effects_per_entity);
          renderer_text(back_buffer, &state->bmp_font_default, state->text_default_color, text_x + ((ARRAY_COUNT("DDP Effects: ON") - 1) * state->bmp_font_default.glyph_width), text_y, text_buffer);
        } else {
          snprintf(text_buffer, ARRAY_COUNT(text_buffer), "DDP Effects: ");
          renderer_text(back_buffer, &state->bmp_font_default, state->text_default_color, text_x, text_y, text_buffer);
          snprintf(text_buffer, ARRAY_COUNT(text_buffer), "OFF");
          renderer_text(back_buffer, &state->bmp_font_default, state->text_color_red, text_x + ((ARRAY_COUNT("DDP Effects: ") - 1) * state->bmp_font_default.glyph_width), text_y, text_buffer);
        }
        text_y += text_yspacing;
      }
      
    }
    
  }
  
  /* Engine console drawing */
  {
    if (state->game_console_state.is_on) {
      S32 i, k;
      GameConsoleState *console;
      
      /*console_update_and_draw();*/
      console = &state->game_console_state;
      renderer_filled_rect(back_buffer, back_buffer->width/2.0f, (back_buffer->height*0.475f)/2.0f, CAST(F32) back_buffer->width, back_buffer->height*0.475f, state->game_console_state.color_bg); /* console transparent background */
      
      renderer_filled_rect(back_buffer, back_buffer->width/2.0f, (back_buffer->height*0.475f) + 1, CAST(F32) back_buffer->width, 1, state->game_console_state.color_border); /* console bottom border */
      
      renderer_text(back_buffer, &state->bmp_font_default, console->color_text, 1, back_buffer->height*0.475f - state->bmp_font_default.glyph_height - 1, ">");
#if 1
      renderer_text(back_buffer, &state->bmp_font_default, console->color_text, CAST(F32) ((1 + console->input_last_index) * state->bmp_font_default.glyph_width + 1), back_buffer->height*0.475f - state->bmp_font_default.glyph_height - 1, "|"); /* cursor/caret indication */
#endif
      k = 0;
      for (i = console->buffer_last_index; i >= 0; --i) {
        renderer_text(back_buffer, &state->bmp_font_default, state->game_console_state.color_text, 10, back_buffer->height*0.475f - state->bmp_font_default.glyph_height - (k * state->bmp_font_default.glyph_height - 1), state->game_console_state.buffer[i]);
        ++k;
      }
      
      for (i = 0; i < console->input_last_index; ++i) {
        if (console->input[i] == '\0') {
          break;
        }
        renderer_text(back_buffer, &state->bmp_font_default, state->game_console_state.color_input, 10, back_buffer->height*0.475f - state->bmp_font_default.glyph_height - 1, state->game_console_state.input);
        /* TODO(Fix): bug when buffer is full - trying to draw the null caracter and the bound check seems to be wrong */
      }
    }
  }
}

INTERNAL void level_menu(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory) {
  GameState *state;
  
  state = CAST(GameState *) memory->permanent_address;
  /* Menu level: setup */
  if (!state->game_level.is_initialized) {
    state->game_level.is_initialized = TRUE;
    state->game_level.is_running = TRUE;
    state->game_level.time_elapsed = 0.0f;
    
    v2_zero(&state->opponent.vel);
    v2_zero(&state->player.vel);
    v2_zero(&state->ball.vel);
  }
  
  /* Menu level: update */
  {
    
  }
  
  /* Menu level: rendering */
  {
    /* TODO: Better background clear */
    renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, CAST(F32) back_buffer->width, CAST(F32) back_buffer->height, state->background_color);
    
    {
      S8 *start_game_text = "PRESS ENTER TO START THE GAME!";
      U64 text_len;
      F32 text_pos_x;
      
      text_len = debug_string_len(start_game_text);
      text_pos_x = (back_buffer->width - (text_len * state->bmp_font_default.glyph_width)) / 2.0f;
      renderer_text(back_buffer, &state->bmp_font_default, state->text_default_color, text_pos_x, back_buffer->height/2.0f, start_game_text);
    }
  }
}

INTERNAL void level_test(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory) {
  GameState *state;
  
  state = CAST(GameState *) memory->permanent_address;
  /* Test level: setup */
  if (!state->game_level.is_initialized) {
    state->game_level.is_initialized = TRUE;
    state->game_level.is_running = TRUE;
    state->game_level.time_elapsed = 0.0f;
  }
  
  /* Test level: update */
  {
  }
  
  /* Test level: render */
  {
    
    renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, CAST(F32) back_buffer->width, CAST(F32) back_buffer->height, state->background_color);
    
#if 0
    renderer_text(back_buffer, &state->bmp_font_default, state->color_default_text, 10, back_buffer->height/2.0f, L"a b c d e f g h i j k l m n o p q r s t u v w x y z ");
    renderer_text(back_buffer, &state->bmp_font_default, state->color_default_text, 10, back_buffer->height/2.0f+20, L"A B C D E F G H I J K L M N O P Q R S T U V W X Y Z");
    renderer_text(back_buffer, &state->bmp_font_default, state->color_default_text, 10, back_buffer->height/2.0f+40, L"- = ( ) { } [ ] ~ < > / ? . , ' \" ! @ # $ % & * | \\ * / ");
#endif
  }
}

INTERNAL void level_classic(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory) {
  GameState *state;
  F32 level_bounding_rect_width, level_bounding_rect_height;
  
  state = CAST(GameState *) memory->permanent_address;
  level_bounding_rect_width = CAST(F32) (state->game_level.max_bounding_rect_x - state->game_level.min_bounding_rect_x);
  level_bounding_rect_height = CAST(F32) (state->game_level.max_bounding_rect_y - state->game_level.min_bounding_rect_y);
  
  /* Classic level: setup */
  if (!state->game_level.is_initialized) {
    S32 i;
    
    state->game_debug_state.count_particles = 0;
    state->game_debug_state.count_trails = 0;
    state->game_debug_state.count_ddp_effects_per_entity = 0;
    
    state->game_level.is_initialized = TRUE;
    state->game_level.is_running = TRUE;
    state->game_level.time_elapsed = 0.0f;
    
    v2_zero(&state->player.vel);
    state->player.pos.x = 15;
    state->player.pos.y = level_bounding_rect_height / 2.0f;
    state->player.width = 12;
    state->player.height = 70;
    
    v2_zero(&state->opponent.vel);
    state->opponent.pos.x = level_bounding_rect_width - 15;
    state->opponent.pos.y = level_bounding_rect_height / 2.0f;
    state->opponent.width = 12;
    state->opponent.height = 70;
    
    state->ball.pos.x = level_bounding_rect_width / 2.0f;
    state->ball.pos.y = level_bounding_rect_height / 2.0f;
    {
      F32 vel_x, vel_y;
      S32 sign;
      
      vel_x = random_f32_range(-250.0f, 250.0f);
      sign = SIGN_OF(vel_x);
      vel_x = (ABS(vel_x) < 100) ? (vel_x + (100 * sign)) : (vel_x);
      vel_y = random_f32_range(-110.0f, 110.0f);
      sign = SIGN_OF(vel_y);
      vel_y = (ABS(vel_y) < 50) ? (vel_y + (50 * sign)) : (vel_y);
      state->ball.vel = v2_create(vel_x, vel_y); /* TODO: Check angle between ball and player/opponent, it cannot be too close to 90deg */
    }
    
    state->game_debug_state.count_trails += ARRAY_COUNT(state->ball.ball_data.trails);
    for (i = 0; i < ARRAY_COUNT(state->ball.ball_data.trails); ++i) {
      Trail *trail;
      
      trail = &state->ball.ball_data.trails[i];
      trail->pos = state->ball.pos;
      trail->life = 0.0f;
    }
    /* NOTE: The current memory stack is just for permanent data, if I keep changing
game levels, it will blow up eventually because each level has its own data.
TODO: transient/level memory - it will be cleared every new level begins */
    state->ball.ball_data.particle_system = particle_system_create(memory, FALSE, state->ball.pos, FALSE, 0, 32, 2, 2, state->ball.color);
    state->game_debug_state.count_particles += state->ball.ball_data.particle_system.particles_count;
  }
  
  /* Classic level: update and render */
  if (state->game_level.is_running) { /* level is running */
    /* Clearing some data */
    {
      state->game_debug_state.count_ddp_effects_per_entity = 0;
    }
    
    /* Player movement code */
    {
      Entity *player;
      
      player = &state->player;
      v2_zero(&player->acc);
      
      if (input->player1.enabled && input->player1.up.pressed)    { player->acc.y = -1; }
      if (input->player1.enabled && input->player1.down.pressed)  { player->acc.y = 1;  }
      
      player->acc = v2_mul(player->acc, 5500.0f);
      player->vel = v2_add(player->vel, v2_mul(player->acc, input->dt));
      player->vel = v2_add(player->vel, v2_mul(player->vel, -0.15f));
      player->pos = v2_add(player->pos, v2_mul(player->vel, input->dt));
      
      if (state->game_debug_state.is_ddp_effects_on) {
        player->width = 12 - ABS(player->vel.y * 0.00159f);
        player->height = 70 + ABS(player->vel.y * 0.05f);
        state->game_debug_state.count_ddp_effects_per_entity += 1; /* TODO: Automation */
      }
    }
    
    /* Opponent movement code */
    {
      Entity *opponent;
      
      opponent = &state->opponent;
      v2_zero(&opponent->acc);
      
#if 0
      if ( (state->ball.vel.y < 0) && (state->ball.pos.y < (opponent->pos.y - opponent->height/4.0f)) ) {
        opponent->acc.y = -1.0f;
      } else if ( (state->ball.vel.y > 0) && (state->ball.pos.y > (opponent->pos.y + opponent->height/4.0f)) ) {
        opponent->acc.y = 1.0f;
      }
      
      if ( (ABS(state->ball.pos.y - opponent->pos.y) > opponent->height * 1.5f) ) {
        opponent->acc = v2_mul(opponent->acc, 3500.0f); /* hard: 10500 */
      } else {
        opponent->acc = v2_mul(opponent->acc, 1500.0f); /* hard: 4500, */
      }
      
      opponent->vel = v2_add(opponent->vel, v2_mul(opponent->acc, input->dt));
      opponent->vel = v2_add(opponent->vel, v2_mul(opponent->vel, -0.15f));
      opponent->pos = v2_add(opponent->pos, v2_mul(opponent->vel, input->dt));
      
      
      opponent->width = 12 - ABS(opponent->vel.y * 0.00159f);
      opponent->height = 70 + ABS(opponent->vel.y * 0.05f);
#else
      /* This opponent "movement code" makes the opponent invincible */
      opponent->pos.y = state->ball.pos.y;
#endif
    }
    
    /* Ball movement code - @IMPORTANT: Remember to clamp ball velocity < 'player_width' */
    {
      Entity *ball;
      
      ball = &state->ball;
      v2_zero(&ball->acc);
      
      /* checks if ball velocity mag squared is greater than player width squared - clamp ball velocity, prevent tunneling */
      {
        F32 ball_mag_squared, player_width_squared;
        
        ball_mag_squared = v2_mag_squared( v2_mul(ball->vel, input->dt) );
        player_width_squared = SQUARE(state->player.width);
        if (ball_mag_squared > player_width_squared) {
          ball->vel = v2_add(ball->vel, v2_mul(ball->vel, -0.005f));
          /* TODO: Set ball mag */
#if 0
          ASSERT(0, "Ball is too fast!!!");
#endif
        }
      }
      
      ball->pos = v2_add(ball->pos, v2_mul(ball->vel, input->dt));
      /* NOTE: Update ball trails position */
      {
        
        if (state->game_debug_state.is_trails_on) {
          U64 trails_count;
          Trail *trail;
          S32 i;
          
          trails_count = ARRAY_COUNT(ball->ball_data.trails);
          ball->ball_data.timer_trail_spawner -= input->dt;
          if (ball->ball_data.timer_trail_spawner < 0) {
            ball->ball_data.timer_trail_spawner = 0.000016f;
            if (ball->ball_data.trails_next >= trails_count) {
              ball->ball_data.trails_next = 0;
            }
            trail = &ball->ball_data.trails[ball->ball_data.trails_next];
            trail->pos = ball->pos;
            trail->angle = v2_angle(ball->vel);
            trail->life = 0.25f * 10.0f;
            ball->ball_data.trails_next++;
          }
          for (i = 0; i < trails_count; ++i) {
            trail = &ball->ball_data.trails[i];
            trail->life -= input->dt*10.0f;
            if (trail->life < 0) trail->life = 0;
          }
        }
        
      }
      
      /* Update ball size multiplier */
      if (state->game_debug_state.is_ddp_effects_on) {
        ball->ball_data.size_multiplier -= input->dt * 2;
        ball->ball_data.size_multiplier = CLAMP(ball->ball_data.size_multiplier, 1.0f, 10.0f);
        state->game_debug_state.count_ddp_effects_per_entity += 1; /* TODO: Automation */
      }
    }
    
    /* Axis-aligned Collision - @IMPORTANT: make sure it's above the 'clear_background' */
    {
      /* Player VS Arena collision & resolution  */
      {
        S32 player_top, player_bottom;
        
        player_top = CAST(S32) (state->player.pos.y - (state->player.height / 2.0f));
        if (player_top < 0) {
          state->player.pos.y = state->player.height / 2.0f;
          state->player.vel = v2_add(state->player.vel, v2_mul(state->player.vel, -2.5f));
        }
        
        player_bottom = CAST(S32) (state->player.pos.y + (state->player.height / 2.0f));
        if (player_bottom > level_bounding_rect_height) {
          state->player.pos.y = level_bounding_rect_height - state->player.height/2.0f;
          state->player.vel = v2_add(state->player.vel, v2_mul(state->player.vel, -2.5f));
        }
      }
      
      /* Opponent VS Arena collision & resolution  */
      {
        S32 opponent_hit_point_top, opponent_hit_point_bottom;
        
        opponent_hit_point_top = CAST(S32) (state->opponent.pos.y - (state->opponent.height / 2.0f));
        if (opponent_hit_point_top < 0) {
          state->opponent.pos.y = state->opponent.height / 2.0f;
          state->opponent.vel = v2_add(state->opponent.vel, v2_mul(state->opponent.vel, -2.5f));
        }
        
        opponent_hit_point_bottom = CAST(S32) (state->opponent.pos.y + (state->opponent.height / 2.0f));
        if (opponent_hit_point_bottom > level_bounding_rect_height) {
          state->opponent.pos.y = level_bounding_rect_height - state->opponent.height/2.0f;
          state->opponent.vel = v2_add(state->opponent.vel, v2_mul(state->opponent.vel, -0.5f));
        }
      }
      
      /* Ball VS Arena collision & resolution  */
      {
        Entity *ball;
        S32 ball_hit_point_top, ball_hit_point_bottom, ball_hit_point_left, ball_hit_point_right;
        
        ball = &state->ball;
        ball_hit_point_top = CAST(S32) (ball->pos.y - ball->height/2.0f);
        if (ball_hit_point_top < 0) {
          ball->pos.y = ball->height/2.0f;
          ball->vel.y *= -1;
          ball->ball_data.particle_system.is_ready_for_emission = TRUE;
          if (state->game_debug_state.is_ddp_effects_on) {
            state->ball.ball_data.size_multiplier = BALL_DEFAULT_SIZE_MULTIPLYER;
          }
        }
        
        ball_hit_point_bottom = CAST(S32) (ball->pos.y + ball->height/2.0f);
        if (ball_hit_point_bottom  > level_bounding_rect_height) {
          ball->pos.y = level_bounding_rect_height - ball->height/2.0f;
          ball->vel.y *= -1;
          ball->ball_data.particle_system.is_ready_for_emission = TRUE;
          if (state->game_debug_state.is_ddp_effects_on) {
            state->ball.ball_data.size_multiplier = BALL_DEFAULT_SIZE_MULTIPLYER;
          }
        }
        
        ball_hit_point_left = CAST(S32) (ball->pos.x - ball->width/2.0f);
        if (ball_hit_point_left < 0) {
          ball->pos.x = level_bounding_rect_width/2.0f;
          ball->pos.y = level_bounding_rect_height/2.0f;
          /*state->opponent.player_data.score++;*/
          state->opponent.player_data.score_accumulation = 0.0f;
          state->game_level.is_running = FALSE;
          state->game_level.is_initialized = FALSE;
        }
        
        ball_hit_point_right = CAST(S32) (ball->pos.x + ball->width/2.0f);
        if (ball_hit_point_right > level_bounding_rect_width) {
          ball->pos.x = level_bounding_rect_width/2.0f;
          ball->pos.y = level_bounding_rect_height/2.0f;
          /*state->player.player_data.score++;*/
          state->game_level.is_running = FALSE;
          state->game_level.is_initialized = FALSE;
        }
      }
      
      /* Ball VS Player collision & resolution - Player is to the left side of
the arena */
      {
        B32 is_colliding;
        Entity *player, *ball;
        
        player = &state->player;
        ball = &state->ball;
        is_colliding = collision_aabb_vs_aabb(ball->pos, ball->width, ball->height, player->pos, player->width, player->height);
        if (is_colliding) {
          F32 ball_y_direction;
          
          ball->pos.x = (player->pos.x + player->width/2.0f) + ball->width/2.0f + 1;
          
          if (player->vel.y < 0) {
            ball_y_direction = -1;
            ball->vel.y = ball_y_direction * ABS(ball->vel.y);
          } else if (player->vel.y > 0.001) {
            ball_y_direction = 1;
            ball->vel.y = ball_y_direction * ABS(ball->vel.y);
          }
          
          ball->vel = v2_add(ball->vel, v2_mul(ball->vel, ABS(player->vel.y) * 0.0009f));
          ball->vel.x *= -1;
          ball->ball_data.particle_system.is_ready_for_emission = TRUE;
          if (state->game_debug_state.is_ddp_effects_on) {
            state->ball.ball_data.size_multiplier = BALL_DEFAULT_SIZE_MULTIPLYER;
          }
        }
      }
      
      /* Ball VS Opponent collision & resolution - The opponent is to the right side of
the arena */
      {
        B32 is_colliding;
        Entity *opponent, *ball;
        
        opponent = &state->opponent;
        ball = &state->ball;
        is_colliding = collision_aabb_vs_aabb(ball->pos, ball->width, ball->height, opponent->pos, opponent->width, opponent->height);
        if (is_colliding) {
          F32 ball_y_direction;
          
          ball->pos.x = (opponent->pos.x - opponent->width/2.0f) - ball->width/2.0f - 1;
          
          if (opponent->vel.y < 0) {
            ball_y_direction = -1;
            ball->vel.y = ball_y_direction * ABS(ball->vel.y);
          } else if (opponent->vel.y > 0.001) {
            ball_y_direction = 1;
            ball->vel.y = ball_y_direction * ABS(ball->vel.y);
          }
          
          ball->vel = v2_add(ball->vel, v2_mul(ball->vel, ABS(opponent->vel.y) * 0.0009f));
          ball->vel.x *= -1;
          ball->ball_data.particle_system.is_ready_for_emission = TRUE;
          if (state->game_debug_state.is_ddp_effects_on) {
            state->ball.ball_data.size_multiplier = BALL_DEFAULT_SIZE_MULTIPLYER;
          }
        }
      }
    }
    
    /* @Test -  particles update after collision */
    {
      if (state->game_debug_state.is_particles_on) {
        state->ball.ball_data.particle_system.pos = state->ball.pos;
        particle_system_update(&state->ball.ball_data.particle_system, input->dt);
      }
    }
    
    /* Re-imagining player score / score progress - Using the arena middle line as a 'load bar' to next levels */
    {
      F32 score_rect_base_height;
      
      state->game_level.time_elapsed += input->dt; /* accumulate dt - Maybe do this somewhere else */
      score_rect_base_height = round_f32(level_bounding_rect_height / state->game_level.time_max);
      state->score_rect_height = CAST(F32) (round_f32_to_s32(state->game_level.time_elapsed) * score_rect_base_height);
      if (state->score_rect_height >= level_bounding_rect_height && (state->game_level.time_elapsed >= state->game_level.time_max)) {
        state->game_level.time_elapsed = 0.0f;
        state->game_level.is_running = FALSE;
        state->game_level.is_initialized = FALSE;
        state->game_level.id = LEVEL_ID_END; /* NOTE: Next level - Level transition goes here */
      }
      
      state->score_rect_x = level_bounding_rect_width/2.0f;
      state->score_rect_y = level_bounding_rect_height/2.0f;
      state->score_rect_width = LEVEL_CLASSIC_MIDDLE_LINE_WIDTH;
    }
    
    /* Classic level: rendering */
    {
      GameColor color_middle_line_red, color_middle_line_white, color_score;
      
      color_middle_line_red = color_create_from_hex(0xb22741ff);
      color_middle_line_white = color_create_from_hex(0x8c7f90ff);
      color_score = color_create_from_hex(0xefd081ff);
      
      /* TODO: Better background clear */
      renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, CAST(F32) back_buffer->width, CAST(F32) back_buffer->height, state->background_color);
      
      /* Classic level arena middle line - Red: simulation not running, White: running */
      if (!state->game_level.is_running) {
        renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, LEVEL_CLASSIC_MIDDLE_LINE_WIDTH, CAST(F32) back_buffer->height, color_middle_line_red);
      } else { /* game level is running */
        renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, LEVEL_CLASSIC_MIDDLE_LINE_WIDTH, CAST(F32) back_buffer->height, color_middle_line_white);
      }
      
      /* New score representation */
      renderer_filled_rect(back_buffer, state->score_rect_x, state->score_rect_y, state->score_rect_width, state->score_rect_height, color_score);
      
      /* Ball particle hit */
      {
        if (state->game_debug_state.is_particles_on) {
          renderer_debug_particles(back_buffer, &state->ball.ball_data.particle_system);
        }
      }
      
      /* 'Not so ugly' Ball trail (rect) representation - @IDEIA: change color if it's FAST */
      {
        
        if (state->game_debug_state.is_trails_on) {
          S32 i;
          GameColor color_trail;
          Trail *trail;
          
          for (i = 0; i < ARRAY_COUNT(state->ball.ball_data.trails); ++i) {
            trail = &state->ball.ball_data.trails[i];
            if (trail->life > 0) {
              color_trail = state->ball.color;
              color_trail.a = (trail->life / 20.0f);
              renderer_filled_rotated_rect(back_buffer, trail->pos.x, trail->pos.y, state->ball.width, state->ball.height, rad_to_deg(trail->angle), color_trail);
            }
          }
        }
        
      }
      
      /* Ball (rect) representation - @IDEIA: change color if it's FAST */
      /* renderer_debug_entity(back_buffer, &state->ball); */
      {
        Entity *b;
        
        b = &state->ball;
        renderer_filled_rect(back_buffer, b->pos.x, b->pos.y, b->width * b->ball_data.size_multiplier, b->height * b->ball_data.size_multiplier, b->color);
      }
      
      /* Player (rect) representation - @IDEIA: change color when moving */
      renderer_debug_entity(back_buffer, &state->player);
      
      /* Opponent (rect) representation - @IDEIA: change color when moving */
      renderer_debug_entity(back_buffer, &state->opponent);
    }
  }
}



/* NOTE: Credits? IDK */
INTERNAL void level_end(GameBackBuffer *back_buffer, GameInput *input, GameMemory *memory) {
  GameState *state;
  
  state = CAST(GameState *) memory->permanent_address;
  /* End level: setup */
  if (!state->game_level.is_initialized) {
    state->game_level.is_initialized = TRUE;
    state->game_level.is_running = TRUE;
    state->game_level.time_elapsed = 0.0f;
    state->game_level.time_max = 5.0f;
    
    v2_zero(&state->opponent.vel);
    v2_zero(&state->player.vel);
    v2_zero(&state->ball.vel);
  }
  
  /* End level: update */
  {
    state->game_level.time_elapsed += input->dt;
    if (state->game_level.time_elapsed >= state->game_level.time_max) {
      state->game_level.is_initialized = FALSE;
      state->game_level.is_running = FALSE;
      state->game_level.id = LEVEL_ID_MENU;
    }
  }
  
  /* End level: rendering */
  {
    GameColor end_background_color;
    
    end_background_color = color_create_from_hex(0xf09548ff);
    
    /* TODO: Better background clear */
    renderer_filled_rect(back_buffer, back_buffer->width/2.0f, back_buffer->height/2.0f, CAST(F32) back_buffer->width, CAST(F32) back_buffer->height, end_background_color);
    
    {
      S8 *start_game_text = "YOU WON!";
      U64 text_len;
      F32 text_pos_x;
      
      text_len = debug_string_len(start_game_text);
      text_pos_x = (back_buffer->width - (text_len * state->bmp_font_default.glyph_width)) / 2.0f;
      renderer_text(back_buffer, &state->bmp_font_default, state->text_color_black, text_pos_x, back_buffer->height/2.0f, start_game_text);
    }
  }
}



EXTERN_CLOSE /* } */