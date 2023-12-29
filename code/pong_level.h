#ifndef PONG_LEVEL_H
#define PONG_LEVEL_H

#define LEVEL_CLASSIC_MIDDLE_LINE_WIDTH 3
#define LEVEL_HORIZONTAL_CLASSIC_MIDDLE_LINE_HEIGHT 3

EXTERN_OPEN /* extern "C" { */

typedef enum LevelID LevelID;
enum LevelID {
  LEVEL_ID_NULL,
  LEVEL_ID_TEST,
  LEVEL_ID_CLASSIC,
  LEVEL_ID_HORIZONTAL_CLASSIC,
  LEVEL_ID_END,
  
  LEVEL_ID_COUNT
};

typedef struct GameLevel GameLevel;
struct GameLevel {
  LevelID id;
  S32 min_bounding_rect_x;
  S32 max_bounding_rect_x;
  S32 min_bounding_rect_y;
  S32 max_bounding_rect_y;
  B32 is_initialized;
  B32 is_running;
  F32 time_elapsed; /* in seconds */
  F32 time_max; /* in seconds */
};

EXTERN_CLOSE /* } */

#endif //PONG_LEVEL_H
