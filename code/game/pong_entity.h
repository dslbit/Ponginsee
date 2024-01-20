#ifndef PONG_ENTITY_H
#define PONG_ENTITY_H

EXTERN_OPEN /* extern "C" { */

typedef enum EntityType EntityType;
enum EntityType {
  ENTITY_TYPE_BLANK,
  ENTITY_TYPE_PLAYER,
  
  ENTITY_TYPE_COUNT
};

typedef struct EntityPlayer EntityPlayer;
struct EntityPlayer {
  F32 score_accumulation;
};

typedef struct Trail Trail;
struct Trail {
  V2 pos;
  F32 life;
};

#define BALL_DEFAULT_SIZE_MULTIPLYER (1.65f)
typedef struct EntityBall EntityBall;
struct EntityBall {
  F32 size_multiplier;
  F32 timer_trail_spawner;
  Trail trails[64]; /* circular buffer - it uses the help of 'trails_next' variable */
  S32 trails_next; /* index to the next available trail V2 */
  ParticleSystem particle_system;
};

typedef struct Entity Entity;
struct Entity {
  V2 pos;
  V2 vel;
  V2 acc;
  
  /* NOTE: For now everything will be recty - This is used for collision and rendering */
  F32 width;
  F32 height;
  GameColor color; /* NOTE: For now I don't have bitmaps for entities, so a color makes sense */
  
  EntityType entity_type;
  union {
    EntityPlayer player_data;
    EntityBall ball_data;
  };
};


INTERNAL INLINE Entity entity_create(EntityType type) {
  Entity result = {0};
  
  result.entity_type = type;
  return result;
}

EXTERN_CLOSE /* } */

#endif /* PONG_ENTITY_H */
