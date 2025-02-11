#ifndef PONG_PARTICLE_H
#define PONG_PARTICLE_H

EXTERN_OPEN /* extern "C" { */

typedef struct Particle Particle;
struct Particle {
  V2 acc;
  V2 vel;
  V2 pos;
  F32 angle;
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
  U32 particles_count;
  Particle *particles;
};

/* NOTE: Change particle color here to texture later? */
INTERNAL ParticleSystem particle_system_create(GameMemory *memory, B32 is_ready_for_emission, V2 initial_pos, B32 is_looping, F32 loop_delay, U32 particles_count, F32 particles_width, F32 particles_height, GameColor particles_color) {
  ParticleSystem ps;
  U32 i;
  
  ps.is_ready_for_emission = is_ready_for_emission;
  ps.pos = initial_pos;
  ps.is_looping = is_looping;
  ps.loop_delay = loop_delay;
  ps.particles_count = particles_count;
  ps.particles = game_memory_push_transient(memory, (particles_count * sizeof(Particle)));
  for (i = 0; i < particles_count; ++i) {
    ps.particles[i].pos = ps.pos;
    ps.particles[i].width = particles_width;
    ps.particles[i].height = particles_height;
    ps.particles[i].color = particles_color;
  }
  ps.is_initialized = TRUE;
  return ps;
}

INTERNAL void particle_system_update(ParticleSystem *ps, F32 dt) {
  U32 i;
  Particle *p;
  
#if 0
  state->ball.ball_data.particle_system.is_ready_for_emission = FALSE; /* only true when collision happens with player, opponent and arena */
#endif
  
  if (ps->is_initialized && ps->is_ready_for_emission) {
    for (i = 0; i < ps->particles_count; ++i) {
      p = &ps->particles[i];
      if (p->life <= 0) {
        p->acc.x = random_f32_range(-1110, 1110);
        p->acc.y = random_f32_range(-1110, 1110);
        p->pos = ps->pos;
        p->life = random_f32_range(0, 0.5f);
      }
    }
    ps->is_ready_for_emission = FALSE;
  }
  
  for (i = 0; i < ps->particles_count; ++i) {
    p = &ps->particles[i];
    
    p->vel = v2_add(p->vel, v2_mul(p->acc, dt));
    p->vel = v2_add(p->vel, v2_mul(p->vel, -0.01f));
    p->pos = v2_add(p->pos, v2_mul(p->vel, dt));
    p->angle = v2_angle(p->vel);
    v2_zero(&p->acc);
    p->life -= dt;
    p->color.a = p->life;
  }
}

EXTERN_CLOSE /* } */

#endif /* PONG_PARTICLE_H */
