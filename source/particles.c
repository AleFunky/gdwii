#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "main.h"
#include "particles.h"
#include "math.h"
#include "game.h"
#include <stdio.h>
#include "easing.h"
GRRLIB_texImg *particleTex = NULL;

ParticleTemplate particle_templates[] = {
    [CUBE_DRAG] = {
        .angle = 90, .angleVar = 45,
        .speed = 75, .speedVar = 20,
        .gravity_x = 0, .gravity_y = -300,
        .rel_gravity = FALSE,
        .life = 0.3f, .lifeVar = 0.15f,
        .start_scale = 0.3, .start_scaleVar = 0.1,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {0,0,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {0,0,0,64},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 1, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 100,
        .texture_id = PARTICLE_SQUARE
    },
    [SHIP_TRAIL] = {
        .angle = 90, .angleVar = 45,
        .speed = 15, .speedVar = 4,
        .gravity_x = 0, .gravity_y = -300,
        .rel_gravity = FALSE,
        .life = 0.2f, .lifeVar = 0.15f,
        .start_scale = 0.25, .start_scaleVar = 0.1,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {255,50,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {255,50,0,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 1, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 100,
        .texture_id = PARTICLE_SQUARE
    },
    [HOLDING_SHIP_TRAIL] = {
        .angle = 90, .angleVar = 90,
        .speed = 15, .speedVar = 4,
        .gravity_x = 0, .gravity_y = -300,
        .rel_gravity = FALSE,
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
        .priority = 100,
        .texture_id = PARTICLE_SQUARE
    },
    [SHIP_DRAG] = {
        .angle = 110, .angleVar = 45,
        .speed = 95, .speedVar = 20,
        .gravity_x = -350, .gravity_y = -300,
        .rel_gravity = FALSE,
        .life = 0.1f, .lifeVar = 0.05f,
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
        .priority = 70,
        .texture_id = PARTICLE_SQUARE
    }, 
    [ORB_PARTICLES] = {
        .angle = 0, .angleVar = 360,
        .speed = -75, .speedVar = 20,
        .gravity_x = 0, .gravity_y = -100,
        .rel_gravity = TRUE,
        .life = 0.3f, .lifeVar = 0.15f,
        .start_scale = 0.2, .start_scaleVar = 0.1,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {255,255,0,178},
        .start_colorVar = {0,0,0,63},
        .end_color = {255,255,0,63},
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
        .priority = 15,
        .texture_id = PARTICLE_SQUARE
    }, 
    [PAD_PARTICLES] = {
        .angle = 90, .angleVar = 0,
        .speed = 100, .speedVar = 20,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = FALSE,
        .life = 0.4f, .lifeVar = 0.12f,
        .start_scale = 0.1f, .start_scaleVar = 0.05f,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {255,255,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {0,0,0,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 13, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 15,
        .texture_id = PARTICLE_SQUARE
    }, 
    [GLITTER_EFFECT] = {
        .angle = 270, .angleVar = 0,
        .speed = 0, .speedVar = 20,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = FALSE,
        .life = 1.f, .lifeVar = 0.8f,
        .start_scale = 0.3f, .start_scaleVar = 0.1f,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {255,255,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {0,0,0,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 240, .sourcePosVarY = 160,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 70,
        .texture_id = PARTICLE_SQUARE
    },
    [PORTAL_PARTICLES] = {
        .angle = 0, .angleVar = 80,
        .speed = -75, .speedVar = 0,
        .gravity_x = 0, .gravity_y = -100,
        .rel_gravity = TRUE,
        .life = 0.48f, .lifeVar = 0,
        .start_scale = 0.3, .start_scaleVar = 0.1,
        .end_scale = 0.05, .end_scaleVar = 0,
        .start_color = {255,255,0,127},
        .start_colorVar = {0,0,0,0},
        .end_color = {0,0,0,255},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 0, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 45,
        .maxRadius = 45,
        .priority = 30,
        .texture_id = PARTICLE_SQUARE
    },
    [USE_EFFECT] = {
        .angle = 0, .angleVar = 0,
        .speed = 0, .speedVar = 0,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = FALSE,
        .life = 0.4f, .lifeVar = 0,
        .start_scale = 40, .start_scaleVar = 0,
        .end_scale = 0.0, .end_scaleVar = 0,
        .start_color = {255,255,0,63},
        .start_colorVar = {0,0,0,0},
        .end_color = {255,255,0,255},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 0, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .trifading = TRUE,
        .priority = 100,
        .texture_id = PARTICLE_CIRCLE
    },
    [ORB_HITBOX_EFFECT] = {
        .angle = 0, .angleVar = 0,
        .speed = 0, .speedVar = 0,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = FALSE,
        .life = 0.3f, .lifeVar = 0,
        .start_scale = 0, .start_scaleVar = 0,
        .end_scale = 90, .end_scaleVar = 0,
        .start_color = {200,200,200,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {200,200,200,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 0, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 70,
        .texture_id = PARTICLE_CIRCUNFERENCE
    },
    [P1_TRAIL] = {
        .angle = 0, .angleVar = 0,
        .speed = 0, .speedVar = 0,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = FALSE,
        .life = 0.3f, .lifeVar = 0,
        .start_scale = 0.733333f, .start_scaleVar = 0,
        .end_scale = 0.733333f, .end_scaleVar = 0,
        .start_color = {0,0,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {0,0,0,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 0, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 100,
        .texture_id = PARTICLE_P1_TRAIL
    },
    [UFO_JUMP] = {
        .angle = 0, .angleVar = 180,
        .speed = 80, .speedVar = 4,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = FALSE,
        .life = 0.4f, .lifeVar = 0.15f,
        .start_scale = 0.25, .start_scaleVar = 0.2,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {0,0,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {0,0,0,255},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 5, .sourcePosVarY = 5,
        .rotatePerSecond = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 70,
        .texture_id = PARTICLE_SQUARE
    },
    [UFO_TRAIL] = {
        .angle = 0, .angleVar = 45,
        .speed = 15, .speedVar = 4,
        .gravity_x = 0, .gravity_y = -300,
        .rel_gravity = FALSE,
        .life = 0.3f, .lifeVar = 0.15f,
        .start_scale = 0.25, .start_scaleVar = 0.1,
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
        .priority = 70,
        .texture_id = PARTICLE_SQUARE
    },
    [DEATH_PARTICLES] = {
        .angle = 0, .angleVar = 180,
        .speed = 160, .speedVar = 80,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = FALSE,
        .life = 0.8f, .lifeVar = 0.15f,
        .start_scale = 0.35, .start_scaleVar = 0.2,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {0,0,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {0,0,0,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 5, .sourcePosVarY = 5,
        .rotatePerSecond = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 100,
        .texture_id = PARTICLE_SQUARE
    },
    [DEATH_CIRCLE] = {
        .angle = 0, .angleVar = 0,
        .speed = 0, .speedVar = 0,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = FALSE,
        .life = 0.5f, .lifeVar = 0,
        .start_scale = 0, .start_scaleVar = 0,
        .end_scale = 90, .end_scaleVar = 0,
        .start_color = {255,255,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {255,255,0,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 0, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 100,
        .texture_id = PARTICLE_CIRCLE
    },
    [BREAKABLE_BRICK_PARTICLES] = {
        .angle = 90, .angleVar = 90,
        .speed = 110, .speedVar = 20,
        .gravity_x = 0, .gravity_y = -616,
        .rel_gravity = FALSE,
        .life = 0.5f, .lifeVar = 0,
        .start_scale = 0.25, .start_scaleVar = 0.1,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {255,255,255,255},
        .start_colorVar = {0,0,0,127},
        .end_color = {255,255,255,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 14, .sourcePosVarY = 14,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 70,
        .texture_id = PARTICLE_SQUARE
    },
    [COIN_PARTICLES] = {
        .angle = 270, .angleVar = 0,
        .speed = 70, .speedVar = 10,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = FALSE,
        .life = 0.6f, .lifeVar = 0.15f,
        .start_scale = 0.15, .start_scaleVar = 0.1,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {255,255,0,255},
        .start_colorVar = {0,0,0,63},
        .end_color = {0,0,0,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 3, .sourcePosVarY = 5,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 50,
        .texture_id = PARTICLE_SQUARE
    },
    [COIN_OBJ] = {
        .angle = 90, .angleVar = 7,
        .speed = 400, .speedVar = 10,
        .gravity_x = 0, .gravity_y = -1500,
        .rel_gravity = FALSE,
        .life = 3.f, .lifeVar = 0,
        .start_scale = 0.733333f, .start_scaleVar = 0,
        .end_scale = 0.733333f, .end_scaleVar = 0,
        .start_color = {255,255,255,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {255,255,255,0},
        .end_colorVar = {0,0,0,0},
        .blending = FALSE,
        .sourcePosVarX = 0, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 100,
        .texture_id = PARTICLE_COIN
    },
    [SPEED_PORTAL_AMBIENT] = {
        .angle = 0, .angleVar = 360,
        .speed = 0, .speedVar = 20,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = FALSE,
        .life = 1.f, .lifeVar = 0.8f,
        .start_scale = 0.3f, .start_scaleVar = 0.1f,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {255,255,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {0,0,0,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 0, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 20,
        .texture_id = PARTICLE_SQUARE
    },
    [SPEEDUP] = {
        .angle = 0, .angleVar = 0,
        .speed = 0, .speedVar = 20,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = FALSE,
        .life = 1.f, .lifeVar = 0.8f,
        .start_scale = 0.3f, .start_scaleVar = 0.1f,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {255,255,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {0,0,0,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 160, .sourcePosVarY = 80,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 100,
        .texture_id = PARTICLE_SQUARE
    },
    [DUAL_BALL_HITBOX_EFFECT] = {
        .angle = 0, .angleVar = 0,
        .speed = 0, .speedVar = 0,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = FALSE,
        .life = 0.3f, .lifeVar = 0,
        .start_scale = 0, .start_scaleVar = 0,
        .end_scale = 90, .end_scaleVar = 0,
        .start_color = {200,200,200,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {200,200,200,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 0, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .lock_to_player = TRUE,
        .priority = 100,
        .texture_id = PARTICLE_CIRCUNFERENCE
    },
    [END_WALL_PARTICLES] = {
        .angle = 180, .angleVar = 80,
        .speed = -300, .speedVar = 0,
        .gravity_x = 0, .gravity_y = -100,
        .rel_gravity = TRUE,
        .life = 0.6f, .lifeVar = 0,
        .start_scale = 0.4, .start_scaleVar = 0,
        .end_scale = 0.05, .end_scaleVar = 0,
        .start_color = {0,0,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {0,0,0,255},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 0, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 150,
        .maxRadius = 250,
        .priority = 100,
        .texture_id = PARTICLE_SQUARE
    },
    [END_WALL_COLL_CIRCLE] = {
        .angle = 0, .angleVar = 0,
        .speed = 0, .speedVar = 0,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = FALSE,
        .life = 0.4f, .lifeVar = 0,
        .start_scale = 0, .start_scaleVar = 0,
        .end_scale = 300, .end_scaleVar = 0,
        .start_color = {0,0,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {0,0,0,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 0, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 100,
        .texture_id = PARTICLE_CIRCLE
    },
    [END_WALL_COLL_CIRCUNFERENCE] = {
        .angle = 0, .angleVar = 0,
        .speed = 0, .speedVar = 0,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = FALSE,
        .life = 1.6f, .lifeVar = 0,
        .start_scale = 0, .start_scaleVar = 0,
        .end_scale = 2400, .end_scaleVar = 0,
        .start_color = {0,0,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {0,0,0,255},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 0, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 100,
        .texture_id = PARTICLE_CIRCUNFERENCE
    },
    [END_WALL_COMPLETE_CIRCLES] = {
        .angle = 0, .angleVar = 0,
        .speed = 0, .speedVar = 0,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = FALSE,
        .life = 0.4f, .lifeVar = 0,
        .start_scale = 0, .start_scaleVar = 0,
        .end_scale = 50, .end_scaleVar = 0,
        .start_color = {0,0,0,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {0,0,0,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 0, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 100,
        .texture_id = PARTICLE_CIRCLE
    },
    [END_WALL_FIREWORK] = {
        .angle = 0, .angleVar = 0,
        .speed = 200, .speedVar = 20,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = TRUE,
        .life = 0.5f, .lifeVar = 0,
        .start_scale = 0.5, .start_scaleVar = 0.1,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {255,255,255,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {255,255,255,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 0, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 100,
        .texture_id = PARTICLE_SQUARE
    },
    [END_WALL_TEXT_EFFECT] = {
        .angle = 0, .angleVar = 360,
        .speed = 100, .speedVar = 20,
        .gravity_x = 0, .gravity_y = 0,
        .rel_gravity = TRUE,
        .life = 0.5f, .lifeVar = 0,
        .start_scale = 0.5, .start_scaleVar = 0.1,
        .end_scale = 0, .end_scaleVar = 0,
        .start_color = {255,255,255,255},
        .start_colorVar = {0,0,0,0},
        .end_color = {255,255,255,0},
        .end_colorVar = {0,0,0,0},
        .blending = TRUE,
        .sourcePosVarX = 0, .sourcePosVarY = 0,
        .rotatePerSecond = 0,
        .rotatePerSecondVariance = 0,
        .rotationStart = 0,
        .rotationStartVariance = 0,
        .rotationEnd = 0,
        .rotationEndVariance = 0,
        .minRadius = 0,
        .maxRadius = 0,
        .priority = 100,
        .texture_id = PARTICLE_SQUARE
    }
};

void add_particle(int i, int group_id, float x, float y, GameObject *parent_obj) {
    ParticleTemplate *tpl = &particle_templates[group_id];
    Particle *particles = state.particles;

    float angle = tpl->angle + tpl->angleVar * random_float(-1, 1);
    float speed = tpl->speed + tpl->speedVar * random_float(-1, 1);

    angle = positive_fmod(angle, 360);

    particles[i].velocity_angle = angle;
    particles[i].velocity_magnitude = speed;
    particles[i].rotate_per_second = tpl->rotatePerSecond + tpl->rotatePerSecondVariance * random_float(-1, 1);

    float vx = cosf(DegToRad(angle)) * speed;
    float vy = sinf(DegToRad(angle)) * speed;
    
    float radius = tpl->minRadius;
    if (tpl->maxRadius > tpl->minRadius) {
        radius += (tpl->maxRadius - tpl->minRadius) * random_float(0, 1);
    }

    float angled_var_x = tpl->sourcePosVarY * cosf(DegToRad(angle)) - tpl->sourcePosVarX * sinf(DegToRad(angle));
    float angled_var_y = tpl->sourcePosVarY * sinf(DegToRad(angle)) + tpl->sourcePosVarX * cosf(DegToRad(angle));

    float px = x + radius * cosf(DegToRad(angle)) + angled_var_x * random_float(-1, 1);
    float py = y + radius * sinf(DegToRad(angle)) + angled_var_y * random_float(-1, 1);

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
    particles[i].elapsed = 0;

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
    particles[i].start_scale = (tpl->start_scale + tpl->start_scaleVar * random_float(-1, 1)) * screen_factor_y;
    particles[i].end_scale = (tpl->end_scale + tpl->end_scaleVar * random_float(-1, 1)) * screen_factor_y;
    particles[i].scale = particles[i].start_scale;
    particles[i].scale_delta = (particles[i].end_scale - particles[i].start_scale) / life;

    particles[i].blending = tpl->blending;
    particles[i].active = TRUE;
    particles[i].rel_gravity = tpl->rel_gravity;
    particles[i].parent_obj = parent_obj;

    particles[i].trifading = tpl->trifading;

    particles[i].texture_id = tpl->texture_id;
    particles[i].lock_to_player = tpl->lock_to_player;
    particles[i].player_id = state.current_player;

    particles[i].priority = tpl->priority;

    particles[i].rotation = 0;

    if (group_id == P1_TRAIL) particles[i].rotation = state.player.lerp_rotation;
}

void spawn_particle(int group_id, float x, float y, GameObject *parent_obj) {
    if (state.paused) return;

    ParticleTemplate *tpl = &particle_templates[group_id];
    Particle *particles = state.particles;

    float priority = tpl->priority;
    int lowest = priority;
    int lowest_index = -1;

    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            add_particle(i, group_id, x, y, parent_obj);
            return;
        } else if (particles[i].priority < priority && particles[i].priority < lowest) {
            // Set as new lowest priority
            lowest = particles[i].priority;
            lowest_index = i;
        }
    }
    // If all full, replace lowest priority
    if (lowest_index >= 0) {
        add_particle(lowest_index, group_id, x, y, parent_obj);
    }
}

void update_particles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &state.particles[i];
        if (p->active) {
            p->velocity_angle += p->rotate_per_second * STEPS_DT;
            if (p->rel_gravity) {
                p->vx += (p->gravity_y * cosf(DegToRad(p->velocity_angle)) - p->gravity_x * sinf(DegToRad(p->velocity_angle))) * STEPS_DT;
                p->vy += (p->gravity_y * sinf(DegToRad(p->velocity_angle)) + p->gravity_x * cosf(DegToRad(p->velocity_angle))) * STEPS_DT;
            } else {
                p->vx += p->gravity_x * STEPS_DT;
                p->vy += p->gravity_y * STEPS_DT;
            }

            p->x += p->vx * STEPS_DT;
            p->y += p->vy * STEPS_DT;

            if (p->lock_to_player) {
                if (p->player_id == 0) {
                    p->x = state.player.x;
                    p->y = state.player.y;
                } else {
                    p->x = state.player2.x;
                    p->y = state.player2.y;
                }
            }

            p->scale = easeValue(EASE_OUT, p->start_scale, p->end_scale, p->elapsed, p->life, 2.f);
            p->color.r += p->color_delta.r * STEPS_DT;
            p->color.g += p->color_delta.g * STEPS_DT;
            p->color.b += p->color_delta.b * STEPS_DT;
            if (p->trifading) {
                if (p->elapsed / p->life < 0.5f) {
                    p->color.a = easeValue(EASE_IN, p->start_color.a, p->end_color.a, p->elapsed, p->life / 2, 2.f);
                } else {
                    p->color.a = easeValue(EASE_OUT, p->end_color.a, p->start_color.a, p->elapsed - (p->life / 2), p->life / 2, 2.f);
                }
            } else {
                p->color.a = easeValue(EASE_OUT, p->start_color.a, p->end_color.a, p->elapsed, p->life, 2.f);
            }
            p->elapsed += STEPS_DT;
            if (p->elapsed >= p->life) {
                p->active = FALSE;
            }
        }
    }
}

void draw_particles(int group_id) {
    GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);
    GRRLIB_texImg *p1TrailTex = get_p1_trail_tex();

    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetVtxDesc(GX_VA_TEX0,   GX_NONE);

    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &state.particles[i];

        if (p->group_id == group_id && p->active) {
            float calc_x = ((p->x - state.camera_x) * SCALE) - widthAdjust;
            float calc_y = screenHeight - ((p->y - state.camera_y) * SCALE);

            if (p->blending) {
                GRRLIB_SetBlend(GRRLIB_BLEND_ADD);
            }
            switch(p->texture_id) { 
                case PARTICLE_SQUARE:
                    custom_rectangle(
                        get_mirror_x(calc_x, state.mirror_factor), calc_y,
                        p->scale * 32, p->scale * 32,
                        RGBA(
                            p->color.r,
                            p->color.g,
                            p->color.b,
                            p->color.a
                        ),
                        TRUE
                    );
                    break;
                case PARTICLE_CIRCLE:
                    custom_circle(
                        get_mirror_x(calc_x, state.mirror_factor), calc_y,
                        p->scale,
                        RGBA(
                            p->color.r,
                            p->color.g,
                            p->color.b,
                            p->color.a
                        )
                    );
                    break;
                case PARTICLE_CIRCUNFERENCE:
                    custom_circunference(
                        get_mirror_x(calc_x, state.mirror_factor), calc_y,
                        p->scale,
                        RGBA(
                            p->color.r,
                            p->color.g,
                            p->color.b,
                            p->color.a
                        ),
                        2.f
                    );
                    break;
                case PARTICLE_P1_TRAIL:
                    set_texture(p1TrailTex);
                    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
                    GX_SetVtxDesc(GX_VA_TEX0,   GX_DIRECT);
                    custom_drawImg(
                        get_mirror_x(calc_x, state.mirror_factor) + 6 - (p1TrailTex->w/2), calc_y + 6 - (p1TrailTex->h/2),
                        p1TrailTex,
                        p->rotation * state.mirror_mult,
                        p->scale * state.mirror_mult, p->scale,
                        RGBA(
                            p->color.r,
                            p->color.g,
                            p->color.b,
                            p->color.a
                        )
                    
                    );
                    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
                    GX_SetVtxDesc(GX_VA_TEX0,   GX_NONE);
                    break;
                case PARTICLE_COIN:
                    GRRLIB_texImg *coin_tex = get_coin_particle_texture();
                    set_texture(coin_tex);
                    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
                    GX_SetVtxDesc(GX_VA_TEX0,   GX_DIRECT);
                    custom_drawImg(
                        get_mirror_x(calc_x, state.mirror_factor) + 6 - (coin_tex->w/2), calc_y + 6 - (coin_tex->h/2),
                        coin_tex,
                        p->rotation * state.mirror_mult,
                        p->scale * state.mirror_mult, p->scale,
                        RGBA(
                            p->color.r,
                            p->color.g,
                            p->color.b,
                            p->color.a
                        )
                    
                    );
                    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
                    GX_SetVtxDesc(GX_VA_TEX0,   GX_NONE);
                    break;
            }
            
            GRRLIB_SetBlend(GRRLIB_BLEND_ALPHA);
        }
    }
    set_texture(prev_tex);
    GX_SetVtxDesc(GX_VA_TEX0,   GX_DIRECT);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
}

void draw_obj_particles(int group_id, GameObject *parent_obj) {
    GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);
    int fade_x = 0;
    int fade_y = 0;

    float fade_scale = 1.f;
    
    float x = ((parent_obj->x - state.camera_x) * SCALE) - widthAdjust;
    get_fade_vars(parent_obj, x, &fade_x, &fade_y, &fade_scale);

    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetVtxDesc(GX_VA_TEX0,   GX_NONE);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &state.particles[i];

        bool condition = (parent_obj == NULL ? TRUE : parent_obj == p->parent_obj);

        if (p->group_id == group_id && condition && p->active) {
            float calc_x = ((p->x - state.camera_x) * SCALE) - widthAdjust;
            float calc_y = screenHeight - ((p->y - state.camera_y) * SCALE);

            if (p->blending) {
                GRRLIB_SetBlend(GRRLIB_BLEND_ADD);
            }

            switch(p->texture_id) { 
                case PARTICLE_SQUARE:
                    custom_rectangle(
                        get_mirror_x(calc_x, state.mirror_factor) + 6 + fade_x, calc_y + 6 + fade_y,
                        p->scale * 32, p->scale * 32,
                        RGBA(
                            p->color.r,
                            p->color.g,
                            p->color.b,
                            p->color.a * (get_fade_value(x, screenWidth) / 255.f) * parent_obj->opacity
                        ), TRUE
                    );
                    break;
                case PARTICLE_CIRCLE:
                    custom_circle(
                        get_mirror_x(calc_x, state.mirror_factor) + 6 + fade_x, calc_y + 6 + fade_y,
                        p->scale,
                        RGBA(
                            p->color.r,
                            p->color.g,
                            p->color.b,
                            p->color.a * (get_fade_value(x, screenWidth) / 255.f) * parent_obj->opacity
                        )
                    );
                    break;
                case PARTICLE_CIRCUNFERENCE:
                    custom_circunference(
                        get_mirror_x(calc_x, state.mirror_factor) + 6 + fade_x, calc_y + 6 + fade_y,
                        p->scale,
                        RGBA(
                            p->color.r,
                            p->color.g,
                            p->color.b,
                            p->color.a * (get_fade_value(x, screenWidth) / 255.f) * parent_obj->opacity
                        ),
                        2
                    );
                    break;
            }
            GRRLIB_SetBlend(GRRLIB_BLEND_ALPHA);
        }
    }
    GX_SetVtxDesc(GX_VA_TEX0,   GX_DIRECT);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
}