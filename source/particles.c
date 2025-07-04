#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "main.h"
#include "particles.h"
#include "math.h"

GRRLIB_texImg *particleTex = NULL;

ParticleTemplate particle_templates[] = {
    [CUBE_DRAG] = {
        .angle = 90, .angleVar = 45,
        .speed = 75, .speedVar = 20,
        .gravity_x = 0, .gravity_y = -300,
        .life = 0.3f, .lifeVar = 0.15f,
        .start_scale = 0.25, .start_scaleVar = 0.2,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {0,0,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {0,0,0,255},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 1, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .maxParticles = 30
    },
    [SHIP_TRAIL] = {
        .angle = 90, .angleVar = 45,
        .speed = 15, .speedVar = 4,
        .gravity_x = 0, .gravity_y = -300,
        .life = 0.3f, .lifeVar = 0.15f,
        .start_scale = 0.25, .start_scaleVar = 0.2,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {255,50,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {255,50,0,255},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 1, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .maxParticles = 30
    },
    [HOLDING_SHIP_TRAIL] = {
        .angle = 90, .angleVar = 90,
        .speed = 15, .speedVar = 4,
        .gravity_x = 0, .gravity_y = -300,
        .life = 0.3f, .lifeVar = 0.15f,
        .start_scale = 0.25, .start_scaleVar = 0.2,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {255,127,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {255,127,0,255},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 1, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .maxParticles = 30
    },
    [SHIP_DRAG] = {
        .angle = 110, .angleVar = 45,
        .speed = 95, .speedVar = 20,
        .gravity_x = -350, .gravity_y = -300,
        .life = 0.3f, .lifeVar = 0.15f,
        .start_scale = 0.15, .start_scaleVar = 0.1,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {255,255,255,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {255,255,255,0},
        .end_colorVar = {0,0,0,0},
        .blending = FALSE,
        .sourcePosVarX = 9, .sourcePosVarY = 1,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .maxParticles = 40
    }, 
    [ORB_PARTICLES] = {
        .angle = 0, .angleVar = 360,
        .speed = -75, .speedVar = 20,
        .gravity_x = 0, .gravity_y = -300,
        .life = 0.3f, .lifeVar = 0.15f,
        .start_scale = 0.3, .start_scaleVar = 0.1,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {255,255,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {0,0,0,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 3, .sourcePosVarY = 3,
        .rotatePerSecond = 360,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 20,
        .maxRadius = 20,
        .maxParticles = 30
    }
};

void spawn_particle(int group_id, float x, float y, GDObjectTyped *parent_obj) {
    ParticleTemplate *tpl = &particle_templates[group_id];
    Particle *particles = state.particles;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            float angle = tpl->angle + tpl->angleVar * random_float(-1, 1);
            float speed = tpl->speed + tpl->speedVar * random_float(-1, 1);

            particles[i].velocity_angle = angle;
            particles[i].velocity_magnitude = speed;
            particles[i].rotate_per_second = tpl->rotatePerSecond + tpl->rotatePerSecondVariance * random_float(-1, 1);

            float vx = cosf(DegToRad(angle)) * speed;
            float vy = sinf(DegToRad(angle)) * speed;
            
            float radius = tpl->minRadius;
            if (tpl->maxRadius > tpl->minRadius) {
                radius += (tpl->maxRadius - tpl->minRadius) * random_float(0, 1);
            }
            float px = x + radius * cosf(DegToRad(angle)) + tpl->sourcePosVarX * random_float(-1, 1);
            float py = y + radius * sinf(DegToRad(angle)) + tpl->sourcePosVarY * random_float(-1, 1);

            float life = tpl->life + tpl->lifeVar * random_float(-1, 1);

            ColorAlpha sc = tpl->start_color;
            ColorAlpha scv = tpl->start_colorVar;
            ColorAlpha ec = tpl->end_color;
            ColorAlpha ecv = tpl->end_colorVar;

            particles[i].group_id = group_id;
            particles[i].x = px;
            particles[i].y = py;
            particles[i].vx = vx;
            particles[i].vy = vy;
            particles[i].ax = 0;
            particles[i].ay = 0;
            particles[i].gravity_x = tpl->gravity_x;
            particles[i].gravity_y = tpl->gravity_y;
            particles[i].life = life;
            particles[i].max_life = life;

            // Color interpolation
            particles[i].start_color.r = sc.r + scv.r * random_float(-1, 1);
            particles[i].start_color.g = sc.g + scv.g * random_float(-1, 1);
            particles[i].start_color.b = sc.b + scv.b * random_float(-1, 1);
            particles[i].start_color.a = sc.a + scv.a * random_float(-1, 1);
            particles[i].end_color.r = ec.r + ecv.r * random_float(-1, 1);
            particles[i].end_color.g = ec.g + ecv.g * random_float(-1, 1);
            particles[i].end_color.b = ec.b + ecv.b * random_float(-1, 1);
            particles[i].end_color.a = ec.a + ecv.a * random_float(-1, 1);

            particles[i].color = particles[i].start_color;
            particles[i].color_delta.r = (particles[i].end_color.r - particles[i].start_color.r) / life;
            particles[i].color_delta.g = (particles[i].end_color.g - particles[i].start_color.g) / life;
            particles[i].color_delta.b = (particles[i].end_color.b - particles[i].start_color.b) / life;
            particles[i].color_delta.a = (particles[i].end_color.a - particles[i].start_color.a) / life;

            // Scale interpolation
            particles[i].start_scale = tpl->start_scale + tpl->start_scaleVar * random_float(-1, 1);
            particles[i].end_scale = tpl->end_scale + tpl->end_scaleVar * random_float(-1, 1);
            particles[i].scale = particles[i].start_scale;
            particles[i].scale_delta = (particles[i].end_scale - particles[i].start_scale) / life;

            particles[i].blending = tpl->blending;
            particles[i].active = TRUE;
            particles[i].parent_obj = parent_obj;
            break;
        }
    }
}

void update_particles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &state.particles[i];
        if (p->active) {
            p->vx += p->gravity_x * STEPS_DT;
            p->vy += p->gravity_y * STEPS_DT;

            if (p->rotate_per_second != 0) {
                float dtheta = DegToRad(p->rotate_per_second) * STEPS_DT;
                float old_vx = p->vx, old_vy = p->vy;
                p->vx = old_vx * cosf(dtheta) - old_vy * sinf(dtheta);
                p->vy = old_vx * sinf(dtheta) + old_vy * cosf(dtheta);
            }

            p->x += p->vx * STEPS_DT;
            p->y += p->vy * STEPS_DT;
            p->scale += p->scale_delta * STEPS_DT;
            p->color.r += p->color_delta.r * STEPS_DT;
            p->color.g += p->color_delta.g * STEPS_DT;
            p->color.b += p->color_delta.b * STEPS_DT;
            p->color.a += p->color_delta.a * STEPS_DT;
            p->life -= STEPS_DT;
            if (p->life <= 0 || p->scale <= 0) {
                p->active = FALSE;
            }
        }
    }
}

void draw_particles(int group_id) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &state.particles[i];

        if (p->group_id == group_id && p->active) {
            float calc_x = ((p->x - state.camera_x) * SCALE);
            float calc_y = screenHeight - ((p->y - state.camera_y) * SCALE);

            if (p->blending) {
                GRRLIB_SetBlend(GRRLIB_BLEND_ADD);
            }
            GRRLIB_DrawImg(
                calc_x,
                calc_y,
                particleTex,
                0,
                p->scale,
                p->scale,
                RGBA(
                    p->color.r,
                    p->color.g,
                    p->color.b,
                    p->color.a
                )
            );
            GRRLIB_SetBlend(GRRLIB_BLEND_ALPHA);
        }
    }
}

void draw_obj_particles(int group_id, GDObjectTyped *parent_obj) {
    int fade_x = 0;
    int fade_y = 0;

    float fade_scale = 1.f;
    
    float x = ((parent_obj->x - state.camera_x) * SCALE);
    get_fade_vars(x, &fade_x, &fade_y, &fade_scale);

    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &state.particles[i];

        bool condition = (parent_obj == NULL ? TRUE : parent_obj == p->parent_obj);

        if (p->group_id == group_id && condition && p->active) {
            float calc_x = ((p->x - state.camera_x) * SCALE);
            float calc_y = screenHeight - ((p->y - state.camera_y) * SCALE);

            if (p->blending) {
                GRRLIB_SetBlend(GRRLIB_BLEND_ADD);
            }
            GRRLIB_DrawImg(
                calc_x + fade_x,
                calc_y + fade_y,
                particleTex,
                0,
                p->scale,
                p->scale,
                RGBA(
                    p->color.r,
                    p->color.g,
                    p->color.b,
                    p->color.a * (get_fade_value(x, screenWidth) / 255.f)
                )
            );
            GRRLIB_SetBlend(GRRLIB_BLEND_ALPHA);
        }
    }
}