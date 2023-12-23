#ifndef PONG_LEVEL_H
#define PONG_LEVEL_H

#define LEVEL_CLASSIC_MIDDLE_LINE_WIDTH 3

EXTERN_OPEN /* extern "C" { */

typedef enum LevelID LevelID;
enum LevelID {
  LEVEL_ID_NULL,
  LEVEL_ID_CLASSIC,
  
  LEVEL_ID_COUNT
};

typedef struct GameLevel GameLevel;
struct GameLevel {
  LevelID id;
  S32 min_bounding_rect_x;
  S32 max_bounding_rect_x;
  S32 min_bounding_rect_y;
  S32 max_bounding_rect_y;
  B32 is_running;
  F32 time_elapsed;
};

EXTERN_CLOSE /* } */

#endif //PONG_LEVEL_H
