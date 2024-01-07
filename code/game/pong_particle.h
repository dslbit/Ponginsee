#ifndef PONG_PARTICLE_H
#define PONG_PARTICLE_H

EXTERN_OPEN /* extern "C" { */

typedef struct Particle Particle;
struct Particle {
  V2 acc;
  V2 vel;
  V2 pos;
  F32 life;
  GameColor color; /* NOTE: maybe use texture in the future instead of color, or mantein color for debug pourpuse */
};

typedef struct ParticleSystem ParticleSystem;
struct ParticleSystem {
  B32 is_initialized;
  B32 is_ready_for_emission;
  V2 pos;
  B32 is_looping;
  F32 loop_delay;
  U64 particles_count;
  Particle *particles;
};

INTERNAL ParticleSystem particle_system_create(GameMemory *mem, B32 is_ready_for_emission, V2 initial_pos, B32 is_looping, F32 loop_delay, U64 particles_count) {
  ParticleSystem ps;
  
  ps.is_ready_for_emission = is_ready_for_emission;
  ps.pos = initial_pos;
  ps.is_looping = is_looping;
  ps.loop_delay = loop_delay;
  ps.particles_count = particles_count;
  ps.particles = game_memory_push(mem, (particles_count * sizeof(Particle)));
  ps.is_initialized = TRUE;
  return ps;
}

EXTERN_CLOSE /* } */

#endif /* PONG_PARTICLE_H */
