#ifndef PONG_PARTICLE_H
#define PONG_PARTICLE_H

EXTERN_OPEN /* extern "C" { */

typedef struct Particle Particle;
struct Particle {
  V2 acc;
  V2 vel;
  V2 pos;
  F32 life;
  F32 width;
  F32 height;
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

/* NOTE: Change particle color here to texture later? */
INTERNAL ParticleSystem particle_system_create(GameMemory *mem, B32 is_ready_for_emission, V2 initial_pos, B32 is_looping, F32 loop_delay, U64 particles_count, F32 particles_width, F32 particles_height, GameColor particles_color) {
  ParticleSystem ps;
  S32 i;
  
  ps.is_ready_for_emission = is_ready_for_emission;
  ps.pos = initial_pos;
  ps.is_looping = is_looping;
  ps.loop_delay = loop_delay;
  ps.particles_count = particles_count;
  ps.particles = game_memory_push(mem, (particles_count * sizeof(Particle)));
  for (i = 0; i < particles_count; ++i) {
    /* ps.particles[i].vel = ; */ /* TODO: Random number generator */
    ps.particles[i].pos = ps.pos;
    ps.particles[i].life = 1.0f;
    ps.particles[i].width = particles_width;
    ps.particles[i].height = particles_height;
    ps.particles[i].color = particles_color;
  }
  ps.is_initialized = TRUE;
  return ps;
}

INTERNAL void particle_system_update(ParticleSystem *ps) {
  S32 i;
  Particle *p;
  
  /* TODO: make use of particle's life & pos integration */
  for (i = 0; i < ps->particles_count; ++i) {
    p = &ps->particles[i];
    p->pos = ps->pos;
  }
}

EXTERN_CLOSE /* } */

#endif /* PONG_PARTICLE_H */
