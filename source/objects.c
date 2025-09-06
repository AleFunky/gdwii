#include <grrlib.h>
#include "objects.h"
#include "level_loading.h"
#include "stdio.h"
#include "object_includes.h"
#include "ground_line_png.h"
#include "levelCompleteText_png.h"
#include "main.h"
#include "math.h"
#include <math.h>
#include "game.h"
#include "custom_mp3player.h"
#include "player.h"
#include "trail.h"
#include "level.h"
#include "collision.h"
#include "easing.h"

#include <wiiuse/wpad.h>
#include "particles.h"
#include "particle_png.h"
#include <ogc/lwp_watchdog.h>

#include "groups.h"

GRRLIB_texImg *prev_tex = NULL;
int prev_blending = GRRLIB_BLEND_ALPHA;

GRRLIB_texImg *current_coin_texture[4];

const int dual_gamemode_heights[GAMEMODE_COUNT] = {
    9,  // Cube
    10, // Ship
    9,  // Ball
    10, // Ufo
    10, // Wave
};

const float jump_heights_table[SPEED_COUNT][JUMP_TYPES_COUNT][GAMEMODE_COUNT][2] = {
    { // SLOW               CUBE                   SHIP                  BALL                    UFO                 WAVE
    /* YELLOW PAD */ {{864,      691.2},    {432,      508.248},  {518.4,       414.72002},   {573.48,   458.784},  {0, 0}},
    /* YELLOW ORB */ {{573.48,   458.784},  {573.48,   458.784},  {401.435993,  321.148795},  {573.48,   458.784},  {0, 0}},
    /* BLUE PAD   */ {{-345.6,   -276.48},  {-229.392, -183.519}, {-160.574397, -128.463298}, {-229.392, -183.519}, {0, 0}},
    /* BLUE ORB   */ {{-229.392, -183.519}, {-229.392, -183.519}, {-160.574397, -128.463298}, {-229.392, -183.519}, {0, 0}},
    /* PINK PAD   */ {{561.6,    449.28},   {302.4,    241.92},   {362.88001,   290.30401},   {345.6,    276.4},    {0, 0}},
    /* PINK ORB   */ {{412.884,  330.318},  {212.166,  169.776},  {309.090595,  247.287596},  {240.84,   192.672},  {0, 0}},
    },
    { // NORMAL
    /* YELLOW PAD */ {{864,      691.2},    {432,      508.248},  {518.4,       414.72002},   {432,      691.2},    {0, 0}},
    /* YELLOW ORB */ {{603.72,   482.976},  {603.72,   482.976},  {422.60399,   338.08319},   {603.72,   482.976},  {0, 0}},
    /* BLUE PAD   */ {{-345.6,   -276.48},  {-345.6,   -276.48},  {-207.36001,  -165.88801},  {-345.6,   -276.48},  {0, 0}},
    /* BLUE ORB   */ {{-241.488, -193.185}, {-241.488, -193.18},  {-169.04160,  -135.2295},   {-241.488, -193.185}, {0, 0}},
    /* PINK PAD   */ {{561.6,    449.28},   {302.4,    241.92},   {362.88001,   290.30401},   {345.6,    276.4},    {0, 0}},
    /* PINK ORB   */ {{434.7,    347.76},   {223.398,  178.686},  {325.42019,   260.3286},    {258.984,  207.198},  {0, 0}},
    },
    { // FAST
    /* YELLOW PAD */ {{864,      691.2},    {432,      508.248},  {518.4,       414.72002},   {432,      691.2},    {0, 0}},
    /* YELLOW ORB */ {{616.68,   481.734},  {616.68,   481.734},  {431.67599,   345.34079},   {616.68,   481.734},  {0, 0}},
    /* BLUE PAD   */ {{-345.6,   -276.48},  {-345.6,   -276.48},  {-207.36001,  -165.88801},  {-345.6,   -276.48},  {0, 0}},
    /* BLUE ORB   */ {{-246.672, -197.343}, {-246.672, -197.343}, {-172.6704,   -138.1401},   {-246.672, -197.343}, {0, 0}},
    /* PINK PAD   */ {{561.6,    449.28},   {302.4,    241.92},   {362.88001,   290.30401},   {345.6,    276.4},    {0, 0}},
    /* PINK ORB   */ {{443.988,  355.212},  {228.15,   182.52},   {332.37539,   265.923},     {258.984,  207.198},  {0, 0}},
    },
    { // FASTER
    /* YELLOW PAD */ {{864,      691.2},    {432,      508.248},  {518.4,       414.72002},   {432,      691.2},    {0, 0}},
    /* YELLOW ORB */ {{606.42,   485.136},  {606.42,   485.136},  {424.493993,  339.59519},   {606.42,   485.136},  {0, 0}},
    /* BLUE PAD   */ {{-345.6,   -276.48},  {-345.6,   -276.48},  {-207.36001,  -165.88801},  {-345.6,   -276.48},  {0, 0}},
    /* BLUE ORB   */ {{-242.568, -194.049}, {-242.568, -194.049}, {-169.7976,   -135.8343},   {-242.568, -194.049}, {0, 0}},
    /* PINK PAD   */ {{561.6,    449.28},   {302.4,    241.92},   {362.88001,   290.30401},   {345.6,    276.4},    {0, 0}},
    /* PINK ORB   */ {{436.644,  349.272},  {224.37,   179.496},  {326.85659,   261.5004},    {254.718,  203.742},  {0, 0}},
    }
};

struct ColorChannel channels[COL_CHANNEL_COUNT];

float get_mirror_x(float x, float factor) {
    return x + factor * (screenWidth - 2.0f * x);
}

void set_intended_ceiling() {
    float mid_point = (state.ground_y + state.ceiling_y) / 2;
    state.camera_intended_y = mid_point - (SCREEN_HEIGHT_AREA / 2);
}

void set_particle_color(int template_id, int r, int g, int b) {
    particle_templates[template_id].start_color.r = r;
    particle_templates[template_id].start_color.g = g;
    particle_templates[template_id].start_color.b = b;

    particle_templates[template_id].end_color.r = r;
    particle_templates[template_id].end_color.g = g;
    particle_templates[template_id].end_color.b = b;
}

void handle_special_hitbox(Player *player, GameObject *obj, ObjectHitbox *hitbox) {
    switch (obj->id) {
        case YELLOW_PAD:
            if (!obj->activated[state.current_player]) {
                MotionTrail_ResumeStroke(&trail);
                player->vel_y = jump_heights_table[state.speed][JUMP_YELLOW_PAD][player->gamemode][player->mini];
                player->on_ground = FALSE;
                player->inverse_rotation = FALSE;
                player->left_ground = TRUE;
                player->ufo_last_y = player->y;
                
                particle_templates[USE_EFFECT].start_scale = 0;
                particle_templates[USE_EFFECT].end_scale = 60;
                particle_templates[USE_EFFECT].trifading = FALSE;

                set_particle_color(USE_EFFECT, 255, 255, 0);
                particle_templates[USE_EFFECT].start_color.a = 255;
                particle_templates[USE_EFFECT].end_color.a = 0;

                spawn_particle(USE_EFFECT, obj->x, obj->y, obj);

                obj->activated[state.current_player] = TRUE;
            }
            break;

        case PINK_PAD:
            if (!obj->activated[state.current_player]) {
                MotionTrail_ResumeStroke(&trail);
                player->vel_y = jump_heights_table[state.speed][JUMP_PINK_PAD][player->gamemode][player->mini];
                player->on_ground = FALSE;
                player->inverse_rotation = FALSE;
                player->left_ground = TRUE;
                player->ufo_last_y = player->y;
                
                particle_templates[USE_EFFECT].start_scale = 0;
                particle_templates[USE_EFFECT].end_scale = 60;
                particle_templates[USE_EFFECT].trifading = FALSE;

                set_particle_color(USE_EFFECT, 255, 31, 255);
                particle_templates[USE_EFFECT].start_color.a = 255;
                particle_templates[USE_EFFECT].end_color.a = 0;

                spawn_particle(USE_EFFECT, obj->x, obj->y, obj);

                obj->activated[state.current_player] = TRUE;
            }
            break;

        case BLUE_PAD:
            if (obj->activated[state.current_player]) player->gravObj = obj;
            else {
                float rotation = adjust_angle(obj->rotation, obj->flippedV, obj->flippedH);
                if ((rotation < 90 || rotation > 270) && player->upside_down)
                    break;
                    
                if ((rotation > 90 && rotation < 270) && !player->upside_down)
                    break;

                MotionTrail_ResumeStroke(&trail);
                if (player->gamemode == GAMEMODE_WAVE) MotionTrail_AddWavePoint(&wave_trail);

                player->left_ground = TRUE;

                player->gravObj = obj;

                player->vel_y = jump_heights_table[state.speed][JUMP_BLUE_PAD][player->gamemode][player->mini];
                player->upside_down ^= 1;
                flip_other_player(state.current_player);
                player->on_ground = FALSE;
                player->inverse_rotation = FALSE;
                player->ufo_last_y = player->y;
                
                particle_templates[USE_EFFECT].start_scale = 0;
                particle_templates[USE_EFFECT].end_scale = 60;
                particle_templates[USE_EFFECT].trifading = FALSE;

                set_particle_color(USE_EFFECT, 56, 200, 255);
                particle_templates[USE_EFFECT].start_color.a = 255;
                particle_templates[USE_EFFECT].end_color.a = 0;

                spawn_particle(USE_EFFECT, obj->x, obj->y, obj);

                obj->activated[state.current_player] = TRUE;
            }
            break;
        
        case YELLOW_ORB:
            if (!obj->activated[state.current_player] && (state.input.holdJump) && player->buffering_state == BUFFER_READY) {    
                MotionTrail_ResumeStroke(&trail);
                
                player->vel_y = jump_heights_table[state.speed][JUMP_YELLOW_ORB][player->gamemode][player->mini];
                
                player->ball_rotation_speed = -1.f;
                
                player->on_ground = FALSE;
                player->on_ceiling = FALSE;
                player->inverse_rotation = FALSE;
                player->left_ground = TRUE;
                player->buffering_state = BUFFER_END;
                player->ufo_last_y = player->y;

                particle_templates[USE_EFFECT].start_scale = 70;
                particle_templates[USE_EFFECT].end_scale = 0;
                particle_templates[USE_EFFECT].trifading = TRUE;

                set_particle_color(USE_EFFECT, 255, 255, 0);
                particle_templates[USE_EFFECT].start_color.a = 0;
                particle_templates[USE_EFFECT].end_color.a = 255;

                spawn_particle(USE_EFFECT, obj->x, obj->y, obj);

                obj->activated[state.current_player] = TRUE;
            } 
            if (!obj->collided[state.current_player]) spawn_particle(ORB_HITBOX_EFFECT, obj->x, obj->y, obj);
            break;
        
        case PINK_ORB:
            if (!obj->activated[state.current_player] && (state.input.holdJump) && player->buffering_state == BUFFER_READY) {    
                MotionTrail_ResumeStroke(&trail);
                
                player->vel_y = jump_heights_table[state.speed][JUMP_PINK_ORB][player->gamemode][player->mini];
                
                player->ball_rotation_speed = -1.f;
                
                player->on_ground = FALSE;
                player->on_ceiling = FALSE;
                player->inverse_rotation = FALSE;
                player->left_ground = TRUE;
                player->buffering_state = BUFFER_END;
                player->ufo_last_y = player->y;

                particle_templates[USE_EFFECT].start_scale = 70;
                particle_templates[USE_EFFECT].end_scale = 0;
                particle_templates[USE_EFFECT].trifading = TRUE;

                set_particle_color(USE_EFFECT, 255, 31, 255);
                particle_templates[USE_EFFECT].start_color.a = 0;
                particle_templates[USE_EFFECT].end_color.a = 255;

                spawn_particle(USE_EFFECT, obj->x, obj->y, obj);

                obj->activated[state.current_player] = TRUE;
            } 
            if (!obj->collided[state.current_player]) spawn_particle(ORB_HITBOX_EFFECT, obj->x, obj->y, obj);
            break;
        
        case BLUE_ORB:
            if (!obj->activated[state.current_player] && (state.input.holdJump) && player->buffering_state == BUFFER_READY) {    
                MotionTrail_ResumeStroke(&trail);
                player->gravObj = obj;
                
                player->vel_y = jump_heights_table[state.speed][JUMP_BLUE_ORB][player->gamemode][player->mini];
                player->upside_down ^= 1;
                if (player->gamemode == GAMEMODE_WAVE) MotionTrail_AddWavePoint(&wave_trail);

                flip_other_player(state.current_player);
                
                player->ball_rotation_speed = -1.f;
                
                player->on_ground = FALSE;
                player->on_ceiling = FALSE;
                player->inverse_rotation = FALSE;
                player->left_ground = TRUE;
                player->buffering_state = BUFFER_END;
                player->ufo_last_y = player->y;

                particle_templates[USE_EFFECT].start_scale = 70;
                particle_templates[USE_EFFECT].end_scale = 0;
                particle_templates[USE_EFFECT].trifading = TRUE;

                set_particle_color(USE_EFFECT, 56, 200, 255);
                particle_templates[USE_EFFECT].start_color.a = 0;
                particle_templates[USE_EFFECT].end_color.a = 255;

                spawn_particle(USE_EFFECT, obj->x, obj->y, obj);

                obj->activated[state.current_player] = TRUE;
            } 
            if (!obj->collided[state.current_player]) spawn_particle(ORB_HITBOX_EFFECT, obj->x, obj->y, obj);
            break;

        case CUBE_PORTAL: 
            if (!obj->activated[state.current_player]) {
                state.ground_y = 0;
                state.ceiling_y = 999999;
                if (player->gamemode != GAMEMODE_CUBE) {
                    if (player->gamemode != GAMEMODE_BALL) {
                        MotionTrail_StopStroke(&trail);
                        player->vel_y /= 2;
                    }

                    if (player->gamemode == GAMEMODE_WAVE) player->vel_y *= 0.9f;
                    
                    player->ceiling_inv_time = 0.1f;
                    player->snap_rotation = TRUE;
                    player->gamemode = GAMEMODE_CUBE;
                    flip_other_player(state.current_player ^ 1);

                    particle_templates[USE_EFFECT].start_scale = 80;
                    particle_templates[USE_EFFECT].end_scale = 0;
                    particle_templates[USE_EFFECT].trifading = TRUE;

                    set_particle_color(USE_EFFECT, 0, 255, 50);
                    particle_templates[USE_EFFECT].start_color.a = 0;
                    particle_templates[USE_EFFECT].end_color.a = 255;

                    spawn_particle(USE_EFFECT, obj->x, obj->y, obj);
                }
                if (state.dual) {
                    set_dual_bounds();
                } 
                obj->activated[state.current_player] = TRUE;
            }
            break;
            
        case SHIP_PORTAL: 
            if (!obj->activated[state.current_player]) {
                state.ground_y = maxf(0, ip1_ceilf((obj->y - 180) / 30.f)) * 30;
                state.ceiling_y = state.ground_y + 300;
                set_intended_ceiling();

                if (player->gamemode != GAMEMODE_SHIP) {
                    if (player->gamemode == GAMEMODE_WAVE) player->vel_y *= 0.9f;
                    player->vel_y /= (player->gamemode == GAMEMODE_UFO || player->gamemode == GAMEMODE_WAVE) ? 4 : 2;
                    
                    player->gamemode = GAMEMODE_SHIP;
                    player->inverse_rotation = FALSE;
                    player->snap_rotation = TRUE;
                    flip_other_player(state.current_player ^ 1);
                    
                    float min = player->mini ? -406.566f : -345.6f;
                    float max = player->mini ? 508.248f : 432.0f;

                    if (player->vel_y < min) {
                        player->vel_y = min;
                    } else if (player->vel_y > max) {
                        player->vel_y = max;
                    }

                    particle_templates[USE_EFFECT].start_scale = 80;
                    particle_templates[USE_EFFECT].end_scale = 0; 
                    particle_templates[USE_EFFECT].trifading = TRUE;

                    set_particle_color(USE_EFFECT, 255, 31, 255);
                    particle_templates[USE_EFFECT].start_color.a = 0;
                    particle_templates[USE_EFFECT].end_color.a = 255;

                    spawn_particle(USE_EFFECT, obj->x, obj->y, obj);
                }
                if (state.dual) {
                    set_dual_bounds();
                } 
                obj->activated[state.current_player] = TRUE;
            }
            break;
        
        case BLUE_GRAVITY_PORTAL:
            player->gravObj = obj;
            if (!obj->activated[state.current_player]) {
                player->ceiling_inv_time = 0.1f;
                if (player->upside_down) {
                    if (player->gamemode != GAMEMODE_BALL) MotionTrail_ResumeStroke(&trail);
                    player->vel_y /= -2;
                    player->upside_down = FALSE;
                    player->inverse_rotation = FALSE;
                    player->snap_rotation = TRUE;
                    if (player->gamemode == GAMEMODE_WAVE) MotionTrail_AddWavePoint(&wave_trail);
                    flip_other_player(state.current_player);
                    player->left_ground = TRUE;
                    player->ufo_last_y = player->y;

                    particle_templates[USE_EFFECT].start_scale = 80;
                    particle_templates[USE_EFFECT].end_scale = 0;
                    particle_templates[USE_EFFECT].trifading = TRUE;

                    set_particle_color(USE_EFFECT, 56, 200, 255);
                    particle_templates[USE_EFFECT].start_color.a = 0;
                    particle_templates[USE_EFFECT].end_color.a = 255;
                    
                    spawn_particle(USE_EFFECT, obj->x, obj->y, obj);
                }
                obj->activated[state.current_player] = TRUE;
            }
            break;
        case YELLOW_GRAVITY_PORTAL:
            player->gravObj = obj;
            if (!obj->activated[state.current_player]) {
                player->ceiling_inv_time = 0.1f;
                if (!player->upside_down) {
                    if (player->gamemode != GAMEMODE_BALL) MotionTrail_ResumeStroke(&trail);
                    player->vel_y /= -2;
                    player->upside_down = TRUE;
                    player->inverse_rotation = FALSE;
                    if (player->gamemode == GAMEMODE_WAVE) MotionTrail_AddWavePoint(&wave_trail);
                    flip_other_player(state.current_player);
                    player->left_ground = TRUE;
                    player->ufo_last_y = player->y;

                    particle_templates[USE_EFFECT].start_scale = 80;
                    particle_templates[USE_EFFECT].end_scale = 0;
                    particle_templates[USE_EFFECT].trifading = TRUE;

                    set_particle_color(USE_EFFECT, 255, 255, 0);
                    particle_templates[USE_EFFECT].start_color.a = 0;
                    particle_templates[USE_EFFECT].end_color.a = 255;
                    
                    spawn_particle(USE_EFFECT, obj->x, obj->y, obj);
                }
            }
            obj->activated[state.current_player] = TRUE;
            break;
        case ORANGE_MIRROR_PORTAL:
            if (!obj->activated[state.current_player]) {
                particle_templates[USE_EFFECT].start_scale = 80;
                particle_templates[USE_EFFECT].end_scale = 0;
                particle_templates[USE_EFFECT].trifading = TRUE;

                set_particle_color(USE_EFFECT, 255, 94, 0);
                particle_templates[USE_EFFECT].start_color.a = 0;
                particle_templates[USE_EFFECT].end_color.a = 255;
                
                spawn_particle(USE_EFFECT, obj->x, obj->y, obj);

                state.intended_mirror_factor = 1.f;
                state.intended_mirror_speed_factor = -1.f;
                obj->activated[state.current_player] = TRUE;
            }
            break;
        case BLUE_MIRROR_PORTAL:
            if (!obj->activated[state.current_player]) {
                particle_templates[USE_EFFECT].start_scale = 80;
                particle_templates[USE_EFFECT].end_scale = 0;
                particle_templates[USE_EFFECT].trifading = TRUE;

                set_particle_color(USE_EFFECT, 56, 200, 255);
                particle_templates[USE_EFFECT].start_color.a = 0;
                particle_templates[USE_EFFECT].end_color.a = 255;
                
                spawn_particle(USE_EFFECT, obj->x, obj->y, obj);

                state.intended_mirror_factor = 0.f;
                state.intended_mirror_speed_factor = 1.f;
                obj->activated[state.current_player] = TRUE;
            }
            break;
        
        case BALL_PORTAL: 
            if (!obj->activated[state.current_player]) {
                state.ground_y = maxf(0, ip1_ceilf((obj->y - 150) / 30.f)) * 30;
                state.ceiling_y = state.ground_y + 240;
                set_intended_ceiling();

                if (player->gamemode != GAMEMODE_BALL) {
                    player->ball_rotation_speed = -1.f;

                    switch (player->gamemode) {
                        case GAMEMODE_WAVE:
                            player->vel_y *= 0.9f;
                            player->vel_y /= 2;
                        case GAMEMODE_SHIP:
                        case GAMEMODE_UFO:
                            player->vel_y /= 2;
                            break;
                    }
                    
                    player->gamemode = GAMEMODE_BALL;
                    player->inverse_rotation = FALSE;
                    player->snap_rotation = TRUE;
                    flip_other_player(state.current_player ^ 1);

                    particle_templates[USE_EFFECT].start_scale = 80;
                    particle_templates[USE_EFFECT].end_scale = 0;
                    particle_templates[USE_EFFECT].trifading = TRUE;

                    set_particle_color(USE_EFFECT, 255, 0, 0);
                    particle_templates[USE_EFFECT].start_color.a = 0;
                    particle_templates[USE_EFFECT].end_color.a = 255;

                    spawn_particle(USE_EFFECT, obj->x, obj->y, obj);
                }
                if (state.dual) {
                    set_dual_bounds();
                } 
                obj->activated[state.current_player] = TRUE;
            }
            break;

        case BIG_PORTAL:
            if (!obj->activated[state.current_player]) {
                player->mini = FALSE;

                particle_templates[USE_EFFECT].start_scale = 80;
                particle_templates[USE_EFFECT].end_scale = 0;
                particle_templates[USE_EFFECT].trifading = TRUE;

                set_particle_color(USE_EFFECT, 0, 255, 50);
                particle_templates[USE_EFFECT].start_color.a = 0;
                particle_templates[USE_EFFECT].end_color.a = 255;

                spawn_particle(USE_EFFECT, obj->x, obj->y, obj);
                
                obj->activated[state.current_player] = TRUE;
            }
            break;        

        case MINI_PORTAL:
            if (!obj->activated[state.current_player]) {
                player->mini = TRUE;

                particle_templates[USE_EFFECT].start_scale = 80;
                particle_templates[USE_EFFECT].end_scale = 0;
                particle_templates[USE_EFFECT].trifading = TRUE;

                set_particle_color(USE_EFFECT, 255, 31, 255);
                particle_templates[USE_EFFECT].start_color.a = 0;
                particle_templates[USE_EFFECT].end_color.a = 255;
                
                spawn_particle(USE_EFFECT, obj->x, obj->y, obj);
                
                obj->activated[state.current_player] = TRUE;
            }
            break;
        case UFO_PORTAL:
            if (!obj->activated[state.current_player]) {
                state.ground_y = maxf(0, ip1_ceilf((obj->y - 180) / 30.f)) * 30;
                state.ceiling_y = state.ground_y + 300;
                set_intended_ceiling();
                
                if (player->gamemode != GAMEMODE_UFO) {
                    if (player->gamemode == GAMEMODE_WAVE) player->vel_y *= 0.9f;
                    player->vel_y /= (player->gamemode == GAMEMODE_SHIP || player->gamemode == GAMEMODE_WAVE) ? 4 : 2;
                    player->gamemode = GAMEMODE_UFO;
                    player->ufo_last_y = player->y;
                    player->inverse_rotation = FALSE;
                    player->snap_rotation = TRUE;
                    flip_other_player(state.current_player ^ 1);

                    if (state.old_player.gamemode == GAMEMODE_CUBE || state.old_player.gamemode == GAMEMODE_SHIP || state.old_player.gamemode == GAMEMODE_WAVE) {
                        player->buffering_state = BUFFER_READY;
                    }

                    particle_templates[USE_EFFECT].start_scale = 80;
                    particle_templates[USE_EFFECT].end_scale = 0;
                    particle_templates[USE_EFFECT].trifading = TRUE;

                    set_particle_color(USE_EFFECT, 255, 127, 0);
                    particle_templates[USE_EFFECT].start_color.a = 0;
                    particle_templates[USE_EFFECT].end_color.a = 255;

                    spawn_particle(USE_EFFECT, obj->x, obj->y, obj);
                }
                if (state.dual) {
                    set_dual_bounds();
                } 
                obj->activated[state.current_player] = TRUE;
            }
            break;
        case SECRET_COIN:
            if (!obj->activated[state.current_player]) {
                // Coin particle
                spawn_particle(COIN_OBJ, obj->x, obj->y, NULL);

                // Explode particles
                particle_templates[BREAKABLE_BRICK_PARTICLES].start_color.a = 127;
                for (s32 i = 0; i < 10; i++) {
                    spawn_particle(BREAKABLE_BRICK_PARTICLES, obj->x, obj->y, obj);
                }

                // Use particles
                particle_templates[USE_EFFECT].start_scale = 60;
                particle_templates[USE_EFFECT].end_scale = 0;
                particle_templates[USE_EFFECT].trifading = FALSE;

                if (level_info.level_is_custom) {
                    set_particle_color(USE_EFFECT, 255, 255, 255);
                } else {
                    set_particle_color(USE_EFFECT, 255, 255, 0);
                }
                particle_templates[USE_EFFECT].start_color.a = 255;
                particle_templates[USE_EFFECT].end_color.a = 0;

                spawn_particle(USE_EFFECT, obj->x, obj->y, obj);
                spawn_particle(ORB_HITBOX_EFFECT, obj->x, obj->y, obj);

                obj->toggled = TRUE;
                obj->activated[state.current_player] = TRUE;
            }
            break;
        case SLOW_SPEED_PORTAL:
            if (!obj->activated[state.current_player]) {
                if (state.speed != SPEED_SLOW) {
                    set_particle_color(SPEEDUP, 255, 220, 0);
                    particle_templates[SPEEDUP].speed = -20;
                    particle_templates[SPEEDUP].sourcePosVarY = 20;
                    for (int i = 0; i < 20; i++) {
                        spawn_particle(SPEEDUP, state.camera_x + SCREEN_WIDTH_AREA + 20, state.camera_y + (SCREEN_HEIGHT_AREA / 2), NULL);
                    }
                }
                spawn_particle(ORB_HITBOX_EFFECT, obj->x, obj->y, obj);
                state.speed = SPEED_SLOW;
                obj->activated[state.current_player] = TRUE;
            }
            break;
        case NORMAL_SPEED_PORTAL:
            if (!obj->activated[state.current_player]) {
                if (state.speed != SPEED_NORMAL) {
                    set_particle_color(SPEEDUP, 0, 255, 255);
                    particle_templates[SPEEDUP].speed = -60;
                    particle_templates[SPEEDUP].sourcePosVarY = 90;
                    for (int i = 0; i < 20; i++) {
                        spawn_particle(SPEEDUP, state.camera_x + SCREEN_WIDTH_AREA + 90, state.camera_y + (SCREEN_HEIGHT_AREA / 2), NULL);
                    }
                }
                spawn_particle(ORB_HITBOX_EFFECT, obj->x, obj->y, obj);
                state.speed = SPEED_NORMAL;
                obj->activated[state.current_player] = TRUE;
            }
            break;
        case FAST_SPEED_PORTAL:
            if (!obj->activated[state.current_player]) {
                if (state.speed != SPEED_FAST) {
                    set_particle_color(SPEEDUP, 64, 255, 64);
                    particle_templates[SPEEDUP].speed = -100;
                    particle_templates[SPEEDUP].sourcePosVarY = 120;
                    for (int i = 0; i < 20; i++) {
                        spawn_particle(SPEEDUP, state.camera_x + SCREEN_WIDTH_AREA + 120, state.camera_y + (SCREEN_HEIGHT_AREA / 2), NULL);
                    }
                }
                spawn_particle(ORB_HITBOX_EFFECT, obj->x, obj->y, obj);
                state.speed = SPEED_FAST;
                obj->activated[state.current_player] = TRUE;
            }
            break;
        case FASTER_SPEED_PORTAL:
            if (!obj->activated[state.current_player]) {
                if (state.speed != SPEED_FASTER) {
                    set_particle_color(SPEEDUP, 255, 127, 255);
                    particle_templates[SPEEDUP].speed = -160;
                    particle_templates[SPEEDUP].sourcePosVarY = 200;
                    for (int i = 0; i < 20; i++) {
                        spawn_particle(SPEEDUP, state.camera_x + SCREEN_WIDTH_AREA + 200, state.camera_y + (SCREEN_HEIGHT_AREA / 2), NULL);
                    }
                }
                spawn_particle(ORB_HITBOX_EFFECT, obj->x, obj->y, obj);
                state.speed = SPEED_FASTER;
                obj->activated[state.current_player] = TRUE;
            }
            break;
        case DUAL_PORTAL:
            player->gravObj = obj;
            if (!obj->activated[state.current_player] && !state.dual) {
                particle_templates[USE_EFFECT].start_scale = 80;
                particle_templates[USE_EFFECT].end_scale = 0;
                particle_templates[USE_EFFECT].trifading = TRUE;

                set_particle_color(USE_EFFECT, 255, 94, 0);
                particle_templates[USE_EFFECT].start_color.a = 0;
                particle_templates[USE_EFFECT].end_color.a = 255;
                
                spawn_particle(USE_EFFECT, obj->x, obj->y, obj);
                
                player->ceiling_inv_time = 0.1f;
                state.dual = TRUE;
                state.dual_portal_y = obj->y;
                setup_dual();
                
                if (player->gamemode == GAMEMODE_WAVE) {
                    MotionTrail_Init(&wave_trail_p2, 3.f, 3, 10.0f, TRUE, p1, trail_tex);   
                    wave_trail_p2.positionR = (Vec2){state.player2.x, state.player2.y};  
                    wave_trail_p2.startingPositionInitialized = TRUE;
                    MotionTrail_AddWavePoint(&wave_trail_p2);
                }
                MotionTrail_Init(&trail_p2, 0.3f, 3, 10.0f, FALSE, p1, trail_tex);

                obj->activated[state.current_player] = TRUE;
            }
            break;

        case DIVORCE_PORTAL:
            if (!obj->activated[state.current_player]) {
                particle_templates[USE_EFFECT].start_scale = 80;
                particle_templates[USE_EFFECT].end_scale = 0;
                particle_templates[USE_EFFECT].trifading = TRUE;

                set_particle_color(USE_EFFECT, 56, 200, 255);
                particle_templates[USE_EFFECT].start_color.a = 0;
                particle_templates[USE_EFFECT].end_color.a = 255;
                
                spawn_particle(USE_EFFECT, obj->x, obj->y, obj);

                state.dual = FALSE;
                obj->activated[state.current_player] = TRUE;
                

                if (state.current_player == 1) {
                    memcpy(&state.player, player, sizeof(Player));
                }
                
                switch (state.player.gamemode) {
                    case GAMEMODE_CUBE:
                        state.ground_y = 0;
                        state.ceiling_y = 999999;
                        break;
                    case GAMEMODE_SHIP:
                    case GAMEMODE_UFO:
                        state.ceiling_y = state.ground_y + 300;
                        set_intended_ceiling();
                        break;
                    case GAMEMODE_BALL:
                        state.ceiling_y = state.ground_y + 240;
                        set_intended_ceiling();
                }
            }
            break;
            
        case WAVE_PORTAL:
            if (!obj->activated[state.current_player]) {
                state.ground_y = maxf(0, ip1_ceilf((obj->y - 180) / 30.f)) * 30;
                state.ceiling_y = state.ground_y + 300;
                set_intended_ceiling();

                if (player->gamemode != GAMEMODE_WAVE) {
                    player->gamemode = GAMEMODE_WAVE;
                    player->inverse_rotation = FALSE;
                    player->snap_rotation = TRUE;
                    flip_other_player(state.current_player ^ 1);
                    wave_trail.positionR = (Vec2){player->x, player->y};  
                    wave_trail.startingPositionInitialized = TRUE;
                    MotionTrail_AddWavePoint(&wave_trail);

                    particle_templates[USE_EFFECT].start_scale = 80;
                    particle_templates[USE_EFFECT].end_scale = 0;
                    particle_templates[USE_EFFECT].trifading = TRUE;

                    set_particle_color(USE_EFFECT, 56, 200, 255);
                    particle_templates[USE_EFFECT].start_color.a = 0;
                    particle_templates[USE_EFFECT].end_color.a = 255;

                    spawn_particle(USE_EFFECT, obj->x, obj->y, obj);
                }
                if (state.dual) {
                    set_dual_bounds();
                } 
                obj->activated[state.current_player] = TRUE;
            }
            break;
    }
    if (!obj->collided[state.current_player]) obj->hitbox_counter[state.current_player]++; 
}

void setup_dual() {
    memcpy(&state.player2, &state.player, sizeof(Player));
    state.player2.upside_down = state.player.upside_down ^ 1;
    set_dual_bounds();
}


void set_dual_bounds() {
    int height = MAX(dual_gamemode_heights[state.player.gamemode],
                 dual_gamemode_heights[state.player2.gamemode]);

    float in_block_y = fmodf(state.dual_portal_y, 30);

    int ground_offset = (ceilf(((float) height + 1) / 2) - 1) * 30;
    state.ground_y = maxf(0, floorf((state.dual_portal_y - ground_offset) / 30.f)) * 30;


    // Shift down if odd height and in the top half
    if (height % 2 != 0) {
        if (in_block_y < 15) {
            state.ground_y = maxf(0, state.ground_y - 30);
        }
    }

    state.ceiling_y = state.ground_y + (height * 30.f);
    set_intended_ceiling();
}

void flip_other_player(int current_player) {
    if (state.dual && state.player.gamemode == state.player2.gamemode && state.player.upside_down == state.player2.upside_down) {
        if (current_player == 0) {
            state.player2.upside_down = !state.player.upside_down;
            state.player2.vel_y /= -2;
            state.player2.ceiling_inv_time = 0.1f;
        } else {
            state.player.upside_down = !state.player2.upside_down;
            state.player.vel_y /= -2;
            state.player.ceiling_inv_time = 0.1f;
        }
    }
}

void do_ball_reflection() {
    Player *player_1 = &state.player;
    Player *player_2 = &state.player2;
    if (state.dual && player_1->gamemode == GAMEMODE_BALL && player_2->gamemode == GAMEMODE_BALL) {
        if (player_1->upside_down == player_2->upside_down) {
            bool ballsIntersecting = intersect(
                player_1->x, player_1->y, player_1->width, player_1->height, 0,
                player_2->x, player_2->y, player_2->width, player_2->height, 0
            );

            if (ballsIntersecting) {
                int current_player = state.current_player;
                if (player_1->on_ground || player_1->on_ceiling) {
                    state.current_player = 0;

                    set_particle_color(DUAL_BALL_HITBOX_EFFECT, p1.r, p1.g, p1.b);
                    spawn_particle(DUAL_BALL_HITBOX_EFFECT, player_1->x, player_1->y, NULL);

                    player_2->vel_y = jump_heights_table[state.speed][JUMP_BLUE_PAD][player_2->gamemode][player_2->mini];
                    player_2->upside_down ^= 1;
                } else if (player_2->on_ground || player_2->on_ceiling) {
                    state.current_player = 1;

                    set_particle_color(DUAL_BALL_HITBOX_EFFECT, p2.r, p2.g, p2.b);
                    spawn_particle(DUAL_BALL_HITBOX_EFFECT, player_2->x, player_2->y, NULL);

                    player_1->vel_y = jump_heights_table[state.speed][JUMP_BLUE_PAD][player_1->gamemode][player_1->mini];
                    player_1->upside_down ^= 1;
                }
                state.current_player = current_player;
            }
        }
        
    }
}

// Prepare Graphics
GRRLIB_texImg *bg;
GRRLIB_texImg *ground;
GRRLIB_texImg *ground_line;
GRRLIB_texImg *level_complete_texture;
GRRLIB_texImg *object_images[OBJECT_COUNT][MAX_OBJECT_LAYERS]; 

int current_fading_effect = FADE_NONE;

bool p1_trail = FALSE;

struct ColTriggerBuffer col_trigger_buffer[COL_CHANNEL_COUNT];
struct MoveTriggerBuffer move_trigger_buffer[MAX_MOVING_CHANNELS];
struct AlphaTriggerBuffer alpha_trigger_buffer[MAX_ALPHA_CHANNELS];

int find_existing_texture(int curr_object, const unsigned char *texture) {
    for (s32 object = 1; object < curr_object; object++) {
        for (s32 layer = 0; layer < MAX_OBJECT_LAYERS; layer++) {
            const unsigned char *loaded_texture = objects[object].layers[layer].texture;
            if (loaded_texture == texture) {
                return (object * MAX_OBJECT_LAYERS) + layer;
            }
        }
    }
    return -1;
}

void load_spritesheet() {
    // Load Textures 
    ground_line = GRRLIB_LoadTexturePNG(ground_line_png);
    level_complete_texture = GRRLIB_LoadTexturePNG(levelCompleteText_png);

    for (s32 object = 1; object < OBJECT_COUNT; object++) {
        for (s32 layer = 0; layer < MAX_OBJECT_LAYERS; layer++) {
            // Skip unused layers
            const unsigned char *texture = objects[object].layers[layer].texture;
            if (!texture) continue;

            printf("Loading texture of object %d layer %d\n", object, layer);
            
            int existing = find_existing_texture(object, texture);
            if (existing < 0) {
                GRRLIB_texImg *image = GRRLIB_LoadTexturePNG((const u8 *) texture);
                if (image == NULL || image->data == NULL) {
                    printf("Couldn't load texture of object %d layer %d\n", object, layer);
                } else {
                    printf("Loaded texture of object %d layer %d\n", object, layer);
                    GRRLIB_SetHandle(image, (image->w/2), (image->h/2));
                    object_images[object][layer] = image;
                }
            } else {
                int object_found = existing / MAX_OBJECT_LAYERS;
                int layer_found = existing % MAX_OBJECT_LAYERS;

                printf("Texture already loaded in object %d layer %d\n", object_found, layer_found);
                object_images[object][layer] = object_images[object_found][layer_found];
            }
        }
    }

    load_icons();
}

void unload_spritesheet() {
    // Free all memory used by textures.
    GRRLIB_FreeTexture(ground_line);
    GRRLIB_FreeTexture(big_font_text);

    for (s32 object = 0; object < OBJECT_COUNT; object++) {
        for (s32 layer = 0; layer < objects[object].num_layers; layer++) {
            const unsigned char *texture = objects[object].layers[layer].texture;
            int existing = find_existing_texture(object, texture);
            // Dont double free textures
            if (existing < 0) {
                GRRLIB_FreeTexture(object_images[object][layer]);
            }
        }
    }
    
    unload_icons();
}
void handle_post_draw_object_particles(GameObject *obj, GDObjectLayer *layer) {
    switch (obj->id) {
        case SLOW_SPEED_PORTAL:
            particle_templates[SPEED_PORTAL_AMBIENT].sourcePosVarX = objects[SLOW_SPEED_PORTAL].hitbox.width / 2;
            particle_templates[SPEED_PORTAL_AMBIENT].sourcePosVarY = objects[SLOW_SPEED_PORTAL].hitbox.height / 2;
            set_particle_color(SPEED_PORTAL_AMBIENT, 255, 220, 0);
            if ((frameCount & 0b1111) == 0) spawn_particle(SPEED_PORTAL_AMBIENT, obj->x, obj->y, obj);
            draw_obj_particles(ORB_HITBOX_EFFECT, obj);
            draw_obj_particles(SPEED_PORTAL_AMBIENT, obj);
            break;
        case NORMAL_SPEED_PORTAL:
            particle_templates[SPEED_PORTAL_AMBIENT].sourcePosVarX = objects[NORMAL_SPEED_PORTAL].hitbox.width / 2;
            particle_templates[SPEED_PORTAL_AMBIENT].sourcePosVarY = objects[NORMAL_SPEED_PORTAL].hitbox.height / 2;
            set_particle_color(SPEED_PORTAL_AMBIENT, 0, 255, 255);
            if ((frameCount & 0b1111) == 0) spawn_particle(SPEED_PORTAL_AMBIENT, obj->x, obj->y, obj);
            draw_obj_particles(ORB_HITBOX_EFFECT, obj);
            draw_obj_particles(SPEED_PORTAL_AMBIENT, obj);
            break;
        case FAST_SPEED_PORTAL:
            particle_templates[SPEED_PORTAL_AMBIENT].sourcePosVarX = objects[FAST_SPEED_PORTAL].hitbox.width / 2;
            particle_templates[SPEED_PORTAL_AMBIENT].sourcePosVarY = objects[FAST_SPEED_PORTAL].hitbox.height / 2;
            set_particle_color(SPEED_PORTAL_AMBIENT, 64, 255, 64);
            if ((frameCount & 0b1111) == 0) spawn_particle(SPEED_PORTAL_AMBIENT, obj->x, obj->y, obj);
            draw_obj_particles(ORB_HITBOX_EFFECT, obj);
            draw_obj_particles(SPEED_PORTAL_AMBIENT, obj);
            break;
        case FASTER_SPEED_PORTAL:
            particle_templates[SPEED_PORTAL_AMBIENT].sourcePosVarX = objects[FASTER_SPEED_PORTAL].hitbox.width / 2;
            particle_templates[SPEED_PORTAL_AMBIENT].sourcePosVarY = objects[FASTER_SPEED_PORTAL].hitbox.height / 2;
            set_particle_color(SPEED_PORTAL_AMBIENT, 255, 127, 255);
            if ((frameCount & 0b1111) == 0) spawn_particle(SPEED_PORTAL_AMBIENT, obj->x, obj->y, obj);
            draw_obj_particles(ORB_HITBOX_EFFECT, obj);
            draw_obj_particles(SPEED_PORTAL_AMBIENT, obj);
            break;
    }

    if (GRRLIB_Settings.blend != prev_blending) {
        GRRLIB_SetBlend(prev_blending);
    }
}

void handle_pre_draw_object_particles(GameObject *obj, GDObjectLayer *layer) {
    switch (obj->id) {
        case YELLOW_ORB:
            set_particle_color(ORB_PARTICLES, 255, 255, 0);
            spawn_particle(ORB_PARTICLES, obj->x, obj->y, obj);
            draw_obj_particles(ORB_PARTICLES, obj);
            draw_obj_particles(USE_EFFECT, obj);
            draw_obj_particles(ORB_HITBOX_EFFECT, obj);
            break;
        
        case YELLOW_PAD:
            particle_templates[PAD_PARTICLES].angle = 180.f - (adjust_angle_y(obj->rotation, obj->flippedV) + 90.f);

            set_particle_color(PAD_PARTICLES, 255, 255, 0);
            spawn_particle(PAD_PARTICLES, obj->x, obj->y, obj);
            draw_obj_particles(PAD_PARTICLES, obj);
            draw_obj_particles(USE_EFFECT, obj);
            break;

        case PINK_ORB:
            set_particle_color(ORB_PARTICLES, 255, 31, 255);
            spawn_particle(ORB_PARTICLES, obj->x, obj->y, obj);
            draw_obj_particles(ORB_PARTICLES, obj);
            draw_obj_particles(USE_EFFECT, obj);
            draw_obj_particles(ORB_HITBOX_EFFECT, obj);
            break;

        case PINK_PAD:
            particle_templates[PAD_PARTICLES].angle = 180.f - (adjust_angle_y(obj->rotation, obj->flippedV) + 90.f);

            set_particle_color(PAD_PARTICLES, 255, 31, 255);
            spawn_particle(PAD_PARTICLES, obj->x, obj->y, obj);
            draw_obj_particles(PAD_PARTICLES, obj);
            draw_obj_particles(USE_EFFECT, obj);
            break;

        case BLUE_ORB:
            set_particle_color(ORB_PARTICLES, 56, 200, 255);
            spawn_particle(ORB_PARTICLES, obj->x, obj->y, obj);
            draw_obj_particles(ORB_PARTICLES, obj);
            draw_obj_particles(USE_EFFECT, obj);
            draw_obj_particles(ORB_HITBOX_EFFECT, obj);
            break;
        
        case BLUE_PAD:
            particle_templates[PAD_PARTICLES].angle = 180.f - (adjust_angle_y(obj->rotation, obj->flippedV) + 90.f);
            
            set_particle_color(PAD_PARTICLES, 56, 200, 255);
            spawn_particle(PAD_PARTICLES, obj->x, obj->y, obj);
            draw_obj_particles(PAD_PARTICLES, obj);
            draw_obj_particles(USE_EFFECT, obj);
            break;
            
            
        case YELLOW_GRAVITY_PORTAL:
            if (layer->layerNum == 1) {
                particle_templates[PORTAL_PARTICLES].angle = 180.f - adjust_angle_y(obj->rotation, obj->flippedH);

                set_particle_color(PORTAL_PARTICLES, 255, 255, 0);
                particle_templates[PORTAL_PARTICLES].start_color.a = 127;
                particle_templates[PORTAL_PARTICLES].end_color.a = 255;
                spawn_particle(PORTAL_PARTICLES, obj->x, obj->y, obj);
                draw_obj_particles(PORTAL_PARTICLES, obj);
                draw_obj_particles(USE_EFFECT, obj);
            }
            break;

        case BLUE_GRAVITY_PORTAL:
        case BLUE_MIRROR_PORTAL:
        case DIVORCE_PORTAL:
        case WAVE_PORTAL:
            if (layer->layerNum == 1) {
                particle_templates[PORTAL_PARTICLES].angle = 180.f - adjust_angle_y(obj->rotation, obj->flippedH);

                set_particle_color(PORTAL_PARTICLES, 56, 200, 255);
                particle_templates[PORTAL_PARTICLES].start_color.a = 127;
                particle_templates[PORTAL_PARTICLES].end_color.a = 255;
                spawn_particle(PORTAL_PARTICLES, obj->x, obj->y, obj);
                draw_obj_particles(PORTAL_PARTICLES, obj);
                draw_obj_particles(USE_EFFECT, obj);
            }
            break;
        
        case CUBE_PORTAL:
        case BIG_PORTAL:
            if (layer->layerNum == 1) {
                particle_templates[PORTAL_PARTICLES].angle = 180.f - adjust_angle_y(obj->rotation, obj->flippedH);

                set_particle_color(PORTAL_PARTICLES, 0, 255, 50);
                particle_templates[PORTAL_PARTICLES].start_color.a = 127;
                particle_templates[PORTAL_PARTICLES].end_color.a = 255;
                spawn_particle(PORTAL_PARTICLES, obj->x, obj->y, obj);
                draw_obj_particles(PORTAL_PARTICLES, obj);
                draw_obj_particles(USE_EFFECT, obj);
            }
            break;

        case SHIP_PORTAL:
        case MINI_PORTAL:
            if (layer->layerNum == 1) {
                particle_templates[PORTAL_PARTICLES].angle = 180.f - adjust_angle_y(obj->rotation, obj->flippedH);

                set_particle_color(PORTAL_PARTICLES, 255, 31, 255);
                particle_templates[PORTAL_PARTICLES].start_color.a = 127;
                particle_templates[PORTAL_PARTICLES].end_color.a = 255;
                spawn_particle(PORTAL_PARTICLES, obj->x, obj->y, obj);
                draw_obj_particles(PORTAL_PARTICLES, obj);
                draw_obj_particles(USE_EFFECT, obj);
            }
            break;

        case ORANGE_MIRROR_PORTAL:
        case UFO_PORTAL:
        case DUAL_PORTAL:
            if (layer->layerNum == 1) {
                particle_templates[PORTAL_PARTICLES].angle = 180.f - adjust_angle_y(obj->rotation, obj->flippedH);

                set_particle_color(PORTAL_PARTICLES, 255, 91, 0);
                particle_templates[PORTAL_PARTICLES].start_color.a = 127;
                particle_templates[PORTAL_PARTICLES].end_color.a = 255;
                spawn_particle(PORTAL_PARTICLES, obj->x, obj->y, obj);
                draw_obj_particles(PORTAL_PARTICLES, obj);
                draw_obj_particles(USE_EFFECT, obj);
            }
            break;
        
        case BALL_PORTAL:
            if (layer->layerNum == 1) {
                particle_templates[PORTAL_PARTICLES].angle = 180.f - adjust_angle_y(obj->rotation, obj->flippedH);

                set_particle_color(PORTAL_PARTICLES, 255, 0, 0);
                particle_templates[PORTAL_PARTICLES].start_color.a = 127;
                particle_templates[PORTAL_PARTICLES].end_color.a = 255;
                spawn_particle(PORTAL_PARTICLES, obj->x, obj->y, obj);
                draw_obj_particles(PORTAL_PARTICLES, obj);
                draw_obj_particles(USE_EFFECT, obj);
            }
            break;
        case BREAKABLE_BLOCK:
            if (layer->layerNum == 1) {
                particle_templates[BREAKABLE_BRICK_PARTICLES].start_color.a = 255;
                draw_obj_particles(BREAKABLE_BRICK_PARTICLES, obj);
            }
            break;
        case SECRET_COIN:
            if (!obj->activated[state.current_player]) {
                if (level_info.level_is_custom) {
                    set_particle_color(COIN_PARTICLES, 255, 255, 255);
                } else {
                    set_particle_color(COIN_PARTICLES, 255, 255, 0);
                }
                
                spawn_particle(COIN_PARTICLES, obj->x - 2, obj->y, obj);
                spawn_particle(COIN_PARTICLES, obj->x - 2, obj->y, obj);
                spawn_particle(COIN_PARTICLES, obj->x - 2, obj->y, obj);
                draw_obj_particles(COIN_PARTICLES, obj);
            } else {
                draw_obj_particles(BREAKABLE_BRICK_PARTICLES, obj);
                draw_obj_particles(USE_EFFECT, obj);
                draw_obj_particles(ORB_HITBOX_EFFECT, obj);
            }
            break;
    }
}

int get_fade_value(float x, int right_edge) {
    if (x < 0 || x > right_edge)
        return 0;
    else if (x < FADE_WIDTH)
        return (int)(255.0f * (x / FADE_WIDTH));
    else if (x > right_edge - FADE_WIDTH)
        return (int)(255.0f * ((right_edge - x) / FADE_WIDTH));
    else
        return 255;
}

int get_xy_fade_offset(float x, int right_edge) {
    int fade = get_fade_value(x, right_edge);
    return (255 - fade) / 2;
}

float get_in_scale_fade(float x, int right_edge) {
    int fade = get_fade_value(x, right_edge);
    return (fade / 255.f);
}

float get_out_scale_fade(float x, int right_edge) {
    int fade = 255 - get_fade_value(x, right_edge);
    return 1 + ((fade / 255.f) / 2);
}

void get_fade_vars(GameObject *obj, float x, int *fade_x, int *fade_y, float *fade_scale) {
    switch (obj->transition_applied) {
        case FADE_NONE:
            break;
        case FADE_UP:
            *fade_y = get_xy_fade_offset(x, screenWidth);
            break;
        case FADE_DOWN:
            *fade_y = -get_xy_fade_offset(x, screenWidth);
            break;
        case FADE_RIGHT:
            *fade_x = get_xy_fade_offset(x, screenWidth) * state.mirror_mult;
            break;
        case FADE_LEFT:
            *fade_x = -get_xy_fade_offset(x, screenWidth) * state.mirror_mult;
            break;
        case FADE_SCALE_IN:
            *fade_scale = get_in_scale_fade(x, screenWidth);
            break;
        case FADE_SCALE_OUT:
            *fade_scale = get_out_scale_fade(x, screenWidth);
            break;
        case FADE_UP_SLOW:
            *fade_x = get_xy_fade_offset(x, screenWidth) * state.mirror_mult * ((current_fading_effect == FADE_CIRCLE_RIGHT) ? 1 : -1);
        case FADE_UP_STATIONARY:
            *fade_y = get_xy_fade_offset(x, screenWidth) / 3;
            break;
        case FADE_DOWN_SLOW:
            *fade_x = get_xy_fade_offset(x, screenWidth) * state.mirror_mult * ((current_fading_effect == FADE_CIRCLE_RIGHT) ? 1 : -1);
        case FADE_DOWN_STATIONARY:
            *fade_y = -get_xy_fade_offset(x, screenWidth) / 3;
            break;
    }
}

int layer_pulses(GameObject *obj, GDObjectLayer *layer) {
    switch (obj->id) {
        case YELLOW_ORB:
        case BLUE_ORB:
        case PINK_ORB:
        case PULSING_CIRCLE:
        case PULSING_CIRCUNFERENCE:
        case PULSING_HEART:
        case PULSING_DIAMOND:
        case PULSING_STAR:
        case PULSING_NOTE:
        case PULSING_SQUARE:
        case PULSING_TRIANGLE:
        case PULSING_HEXAGON:
        case D_ARROW:
        case D_EXMARK:
        case D_QMARK:
        case D_CROSS:
        case D_ARROW_2:
        case PULSING_BIG_CIRCUNFERENCE:
        case PULSING_ARROW_3:
        case PULSING_BIG_SQUARE:
        case PULSING_BIG_SQUARE_OUTLINE:
        case PULSING_BIG_CIRCLE:
            return 1;
        case ROD_BIG:
        case ROD_MEDIUM:
        case ROD_SMALL:
            if (layer->layerNum == 1) {
                return 1;
            }
            break;
    }

    return 0;
}

float get_object_pulse(float amplitude, GameObject *obj) {
    switch (obj->id) {
        case YELLOW_ORB:
        case BLUE_ORB:
        case PINK_ORB:
            return map_range(amplitude, 0.f, 1.f, 0.3f, 1.2f);
        case PULSING_CIRCLE:
        case PULSING_CIRCUNFERENCE:
        case PULSING_HEART:
        case PULSING_DIAMOND:
        case PULSING_STAR:
        case PULSING_NOTE:
        case PULSING_SQUARE:
        case PULSING_TRIANGLE:
        case PULSING_HEXAGON:
        case ROD_BIG:
        case ROD_MEDIUM:
        case ROD_SMALL:
            return amplitude;
        case D_ARROW:
        case D_EXMARK:
        case D_QMARK:
        case D_CROSS:
        case D_ARROW_2:
        case PULSING_ARROW_3:
        case PULSING_BIG_SQUARE:
        case PULSING_BIG_SQUARE_OUTLINE:
        case PULSING_BIG_CIRCLE:
        case PULSING_BIG_CIRCUNFERENCE:
            return map_range(amplitude, 0.f, 1.f, 0.6f, 1.2f);
    }
    return amplitude;
}

GRRLIB_texImg *get_randomized_texture(GRRLIB_texImg *image, GameObject *obj, GDObjectLayer *layer) {
    switch (obj->id) {
        case GROUND_SPIKE:
            return object_images[GROUND_SPIKE][obj->random % 3];
        case ROD_BIG:
        case ROD_MEDIUM:
        case ROD_SMALL:
            if (layer->layerNum == 1) {
                return object_images[ROD_BIG][level_info.pulsing_type + 1]; // balls start at 1
            }
            break;
        case SECRET_COIN:
            int index = (frame_counter & 0b1100000) >> 5;
            return current_coin_texture[index];
        case BUSH_GROUND_SPIKE:
            return object_images[BUSH_GROUND_SPIKE][obj->random & 0b11];
    }

    return image;
}

GRRLIB_texImg *get_coin_particle_texture() {
    int index = (frame_counter & 0b110000) >> 4;
    return current_coin_texture[index];
}

void load_coin_texture() {
    for (s32 i = 0; i < NUM_COIN_FRAMES; i++) {
        if (level_info.level_is_custom) {
            current_coin_texture[i] = GRRLIB_LoadTexturePNG(user_coin_layer[i].texture);
        } else {
            current_coin_texture[i] = GRRLIB_LoadTexturePNG(secret_coin_layer[i].texture);
        }
        GRRLIB_SetHandle(current_coin_texture[i], current_coin_texture[i]->w / 2, current_coin_texture[i]->h / 2);
    }
}

void unload_coin_texture() {
    for (s32 i = 0; i < NUM_COIN_FRAMES; i++) {
        if (current_coin_texture[i]) {
            GRRLIB_FreeTexture(current_coin_texture[i]);
            current_coin_texture[i] = NULL;
        }
    }
}

int get_opacity(GameObject *obj, float x) {
    int opacity = get_fade_value(x, screenWidth);

    switch (obj->id) {
        case BLACK_FULL:
        case BLACK_EDGE:
        case BLACK_CORNER:
        case BLACK_INS_CORNER:
        case BLACK_FILLER:
        case BLACK_PILLAR_END:
        case BLACK_PILLAR:
        case BLACK_SLOPE_45:
        case BLACK_SLOPE_22_66:
            if (obj->transition_applied == FADE_NONE) opacity = 255;
            break;
            
        case COLORED_FULL:
        case COLORED_EDGE:
        case COLORED_CORNER:
        case COLORED_INS_CORNER:
        case COLORED_FILLER:
        case COLORED_PILLAR_END:
        case COLORED_PILLAR:
        case COLORED_SLOPE_45:
        case COLORED_SLOPE_22_66:
        case COLOR_SLOPE_45_NOOUT:
        case COLOR_SLOPE_22_66_NOOUT:
            bool blending = channels[obj->object.detail_col_channel].blending;
            if (!blending && obj->transition_applied == FADE_NONE) opacity = 255;
            break;
    }

    return opacity;
}

float get_fading_obj_fade(GameObject *obj, float x, float right_edge) {
    float fading_obj_width = FADING_OBJ_WIDTH * screen_factor_x;
    if (x < FADING_OBJ_PADDING || x > right_edge - FADING_OBJ_PADDING)
        return 1.f;
    else if (x < FADING_OBJ_PADDING + fading_obj_width)
        return clampf(1.f - ((x - FADING_OBJ_PADDING) / fading_obj_width), 0.05f, 1.f);
    else if (x > right_edge - fading_obj_width - FADING_OBJ_PADDING)
        return clampf(1.f - ((right_edge - (x + FADING_OBJ_PADDING)) / fading_obj_width), 0.05f, 1.f);
    else
        return 0.05f;
}

float get_rotation_speed(GameObject *obj) {
    switch (obj->id) {
        case SAW_BIG: 
        case SAW_MEDIUM:
        case SAW_SMALL:
        case BLADE_BIG:
        case BLADE_MEDIUM:
        case BLADE_SMALL:
        case BLADE_ALT_BIG:
        case BLADE_ALT_MEDIUM:
        case BLADE_ALT_SMALL:
        case DARKBLADE_BIG:
        case DARKBLADE_MEDIUM:
        case DARKBLADE_SMALL:
        case DARKCOGWHEEL_BIG:
        case DARKCOGWHEEL_MEDIUM:
        case DARKCOGWHEEL_SMALL:
        case LIGHTBLADE_BIG:
        case LIGHTBLADE_MEDIUM:
        case LIGHTBLADE_SMALL:
        case FADING_BLADE_BIG:
        case FADING_BLADE_MEDIUM:
        case FADING_BLADE_SMALL:
            return 360.f;
        
        case SAW_DECO_BIG:
        case SAW_DECO_MEDIUM:
        case SAW_DECO_SMALL:
        case SAW_DECO_TINY:
        case WHEEL_BIG:
        case WHEEL_MEDIUM:
        case WHEEL_SMALL:
        case SPIKEWHEEL_BIG:
        case SPIKEWHEEL_MEDIUM:
        case SPIKEWHEEL_SMALL:
        case CARTWHEEL_BIG:
        case CARTWHEEL_MEDIUM:
        case CARTWHEEL_SMALL:
        case ROUND_CLOUD_BIG:
        case ROUND_CLOUD_MEDIUM:
        case ROUND_CLOUD_SMALL:
        case ROTATING_BALL_BIG:
        case ROTATING_BALL_MEDIUM:
        case ROTATING_BALL_SMALL:
        case ROTATING_BALL_TINY:
        case ROTATING_HEXAGON_BIG:
        case ROTATING_HEXAGON_MEDIUM:
        case ROTATING_HEXAGON_TINY:
            return 180.f;
    }
    return 0.f;
}

bool is_modifiable(int col_channel) {
    switch(col_channel) {
        case BLACK:
        case WHITE:
        case OBJ_BLENDING:
        case LBG_NO_LERP:
            return FALSE;
    }
    return TRUE;
}

u32 get_layer_color(GameObject *obj, GDObjectLayer *layer, int col_channel, float opacity) {
    Color color;
    color.r = channels[col_channel].color.r;
    color.g = channels[col_channel].color.g;
    color.b = channels[col_channel].color.b;

    if (get_main_channel_id(obj->id) <= 0 && obj->object.main_col_HSV_enabled) {
        // Detail only objects use the main slot
        color = HSV_combine(color, obj->object.main_col_HSV);
    } else if (layer->layer->color_type == COLOR_MAIN && obj->object.main_col_HSV_enabled) {
        color = HSV_combine(color, obj->object.main_col_HSV);
    } else if (layer->layer->color_type == COLOR_DETAIL && obj->object.detail_col_HSV_enabled) {
        color = HSV_combine(color, obj->object.detail_col_HSV);
    }

    return RGBA(color.r, color.g, color.b, opacity * channels[col_channel].alpha * obj->opacity);
}

static inline void put_object_layer(GameObject *obj, float x, float y, GDObjectLayer *layer) {
    int obj_id = obj->id;

    int layer_index = layer->layerNum;

    int x_flip_mult = (obj->flippedH ? -1 : 1);
    int y_flip_mult = (obj->flippedV ? -1 : 1);

    struct ObjectLayer *objectLayer = layer->layer;
    float x_offset = objectLayer->x_offset * x_flip_mult;
    float y_offset = objectLayer->y_offset * y_flip_mult;

    GRRLIB_texImg *tex = get_randomized_texture(object_images[obj_id][layer_index], obj, layer);

    int width = tex->w;
    int height = tex->h;

    int col_channel = layer->col_channel;

    int blending;
    if (channels[col_channel].blending || layer->blending) {
        blending = GRRLIB_BLEND_ADD;
    } else {
        blending = GRRLIB_BLEND_ALPHA;
    }

    int opacity = get_opacity(obj, x);
    int unmodified_opacity = opacity;
    
    if (objects[obj_id].fades) {
        opacity *= get_fading_obj_fade(obj, x, screenWidth);
    }

    u32 color = get_layer_color(obj, layer, col_channel, opacity);

    // If it is invisible because of blending, skip
    if ((blending == GRRLIB_BLEND_ADD && !(color & ~0xff)) || opacity == 0) return;
        
    float angle_rad = DegToRad(obj->rotation); // Convert degrees to radians
    float cos_a = cosf(angle_rad);
    float sin_a = sinf(angle_rad);

    float x_off_rot = (x_offset * cos_a - y_offset * sin_a) * state.mirror_mult;
    float y_off_rot = x_offset * sin_a + y_offset * cos_a;

    int fade_x = 0;
    int fade_y = 0;

    float fade_scale = 1.f;

    get_fade_vars(obj, x, &fade_x, &fade_y, &fade_scale);

    // Get scaling because of music
    if (layer_pulses(obj, layer)) {
        if (level_info.custom_song_id <= 0) {
            obj->ampl_scaling = ease_out(obj->ampl_scaling, get_object_pulse(amplitude, obj), 0.25f);
        } else {
            obj->ampl_scaling = get_object_pulse(amplitude, obj);
        }
        fade_scale *= obj->ampl_scaling;
    }

    float rotation = adjust_angle(obj->rotation, 0, state.mirror_mult < 0);

    // Handle rod ball spawn
    switch(obj_id) {
        case ROD_BIG:
        case ROD_MEDIUM:
        case ROD_SMALL:
            if (layer->layerNum == 1) {
                rotation = 0.f;
                x_flip_mult = 1.f;
                y_flip_mult = 1.f;
            }
    }

    // Handle special fade types
    if (obj->transition_applied == FADE_DOWN_STATIONARY || obj->transition_applied == FADE_UP_STATIONARY) {
        if (unmodified_opacity < 255) {
            if (x > screenWidth / 2) {
                x = screenWidth - FADE_WIDTH;
            } else {
                x = FADE_WIDTH;
            }
        }
    }
    
    if (prev_tex != tex) {
        prev_tex = tex;
        set_texture(tex);
    }
    

    if (blending != prev_blending) {
        GRRLIB_SetBlend(blending);
        prev_blending = blending;
    }

    custom_drawImg(
        /* X        */ get_mirror_x(x, state.mirror_factor) + 6 - (width/2) + x_off_rot + fade_x,
        /* Y        */ y + 6 - (height/2) + y_off_rot + fade_y,
        /* Texture  */ tex, 
        /* Rotation */ rotation, 
        /* Scale X  */ BASE_SCALE * x_flip_mult * fade_scale * state.mirror_mult * obj->object.scale_x, 
        /* Scale Y  */ BASE_SCALE * y_flip_mult * fade_scale * obj->object.scale_y, 
        /* Color    */ color
    );
}


void draw_background_image(f32 x, f32 y, bool vflip) {
    for (s32 i = 0; i < BG_DIMENSIONS; i++) {
        float calc_x = i*BG_CHUNK - widthAdjust;
        for (s32 j = 0; j < BG_DIMENSIONS; j++) {
            float calc_y = j*BG_CHUNK;
            
            GRRLIB_SetHandle(bg, 512, 512);
            custom_drawPart(
                x + calc_x, 
                y + (vflip ? (BG_CHUNK * 3) - calc_y : calc_y), 
                i * 256, 
                j * 256,
                256, 
                256,
                bg, 0, 1 * BACKGROUND_SCALE, (vflip ? -1 : 1) * BACKGROUND_SCALE, RGBA(channels[BG].color.r, channels[BG].color.g, channels[BG].color.b, 255)
            );
        }
    }
}

void draw_background(f32 x, f32 y) {
    GX_SetTevOp  (GX_TEVSTAGE0, GX_MODULATE);
    GX_SetVtxDesc(GX_VA_TEX0,   GX_DIRECT);
    set_texture(bg);

    float offset = 1024 * BACKGROUND_SCALE;
    int tiles_x = (screenWidth / offset) + 2;
    int tiles_y = (screenHeight / offset) + 2;

    float calc_x = positive_fmod(x, offset) - widthAdjust;
    float calc_y = positive_fmod(y, offset);

    for (int i = -1; i < tiles_x; i++) {
        for (int j = -1; j < tiles_y; j++) {
            // Calculate position for each tile
            float draw_x = -calc_x + i * offset;
            float draw_y = -calc_y + j * offset;
            
            // Flip every other row
            bool vflip = ((j % 2) != 0);
            draw_background_image(draw_x, draw_y, vflip);
        }
    }
}

float complete_text_elapsed = 0;
void draw_end_wall() {  
    if (level_info.wall_y > 0) {
        spawn_particle(END_WALL_PARTICLES, level_info.wall_x, level_info.wall_y, NULL);
        spawn_particle(END_WALL_PARTICLES, level_info.wall_x, level_info.wall_y, NULL);
        draw_particles(END_WALL_PARTICLES);
        draw_particles(END_WALL_COLL_CIRCLE);
        draw_particles(END_WALL_COLL_CIRCUNFERENCE);
        draw_particles(END_WALL_COMPLETE_CIRCLES);
        draw_particles(END_WALL_FIREWORK);
        draw_particles(END_WALL_TEXT_EFFECT);

        // Render rays
        draw_rays();

        if (completion_timer > 2) {
            float mult = 0.5625f * screen_factor_x;

            float offset_x = (level_complete_texture->w / 2);
            float offset_y = (level_complete_texture->h / 2) + 50 * screen_factor_y;

            float text_scale = easeValue(ELASTIC_OUT, 0, mult, complete_text_elapsed, COMPLETE_TEXT_IN_TIME, 0.6f);
            GRRLIB_SetHandle(level_complete_texture, level_complete_texture->w / 2, level_complete_texture->h / 2);
            GRRLIB_DrawImg(screenWidth / 2 - offset_x, screenHeight / 2 - offset_y, level_complete_texture, 0, text_scale, text_scale, 0xffffffff);
            complete_text_elapsed += dt;
        }
    }

    float calc_x = ((level_info.wall_x - state.camera_x) * SCALE) - widthAdjust;
    float calc_y =  positive_fmod(state.camera_y * SCALE, BLOCK_SIZE_PX) + screenHeight;  
    GX_SetTevOp  (GX_TEVSTAGE0, GX_MODULATE);
    GX_SetVtxDesc(GX_VA_TEX0,   GX_DIRECT);
    if (level_info.wall_y > 0) {
        for (s32 j = 0; j < objects[CHECKER_EDGE].num_layers; j++) {
            GRRLIB_texImg *image = object_images[CHECKER_EDGE][j];
            int width = image->w;
            int height = image->h;
            set_texture(image);

            // Draw each wall block
            for (float i = -BLOCK_SIZE_PX; i < screenHeight + BLOCK_SIZE_PX * 2; i += BLOCK_SIZE_PX) {
                custom_drawImg(
                    get_mirror_x(calc_x, state.mirror_factor) - (width/2) + 6, 
                    calc_y + 6 - i - (height/2),    
                    image,
                    adjust_angle(270, 0, state.mirror_mult < 0),
                    BASE_SCALE * state.mirror_mult, BASE_SCALE,
                    RGBA(255, 255, 255, 255) 
                );
            }
        }
        GRRLIB_SetBlend(GRRLIB_BLEND_ADD);
        
        calc_x = ((level_info.wall_x - 25 - state.camera_x) * SCALE) - widthAdjust;

        // Draw glow
        GRRLIB_texImg *image = object_images[GLOW][0];
        int width = image->w;
        int height = image->h;
        set_texture(image);
        for (float i = -BLOCK_SIZE_PX; i < screenHeight + BLOCK_SIZE_PX * 2; i += BLOCK_SIZE_PX) {
            custom_drawImg(
                get_mirror_x(calc_x, state.mirror_factor) - (width/2) + 6, 
                calc_y + 6 - i - (height/2),    
                image,
                adjust_angle(270, 0, state.mirror_mult < 0),
                BASE_SCALE * state.mirror_mult, BASE_SCALE,
                RGBA(p1.r, p1.g, p1.b, 255) 
            );
        }
    }   
    GRRLIB_SetBlend(GRRLIB_BLEND_ALPHA);
    GX_SetTevOp  (GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetVtxDesc(GX_VA_TEX0,   GX_NONE);
}

#define GROUND_SIZE 176 // pixels
#define LINE_SCALE 0.5f

void draw_ground(f32 y, bool is_ceiling) {
    int mult = (is_ceiling ? -1 : 1);

    // First draw the ground
    float calc_x = 0 - positive_fmod(state.ground_x * SCALE, GROUND_SIZE);
    float calc_y = screenHeight - ((y - state.camera_y) * SCALE);

    for (float i = -GROUND_SIZE; i < screenWidth + GROUND_SIZE; i += GROUND_SIZE) {
        GRRLIB_DrawImg(
            calc_x + i + 6, 
            calc_y + 6,    
            ground,
            0, 1.375, 1.375 * mult,
            RGBA(channels[GROUND].color.r, channels[GROUND].color.g, channels[GROUND].color.b, 255) 
        );
    }

    // Then draw the line
    if (channels[LINE].blending) {
        GRRLIB_SetBlend(GRRLIB_BLEND_ADD);
    }

    int line_width = ground_line->w * screen_factor_x;
    
    GRRLIB_DrawImg(
        (screenWidth / 2) - (line_width / (2 / LINE_SCALE)),
        calc_y + (is_ceiling ? 4 : 6),
        ground_line,
        0, LINE_SCALE * screen_factor_x, 0.75,
        RGBA(channels[LINE].color.r, channels[LINE].color.g, channels[LINE].color.b, 255)
    );
    
    if (channels[LINE].blending) {
        GRRLIB_SetBlend(GRRLIB_BLEND_ALPHA);
    }
}

void handle_special_fading(GameObject *obj, float calc_x, float calc_y) {
    switch (current_fading_effect) {
        case FADE_INWARDS:
            if (calc_y > (screenHeight / 2)) {
                obj->transition_applied = FADE_UP;
            } else {
                obj->transition_applied = FADE_DOWN;
            }
            break;
        case FADE_OUTWARDS:
            if (calc_y > (screenHeight / 2)) {
                obj->transition_applied = FADE_DOWN;
            } else {
                obj->transition_applied = FADE_UP;
            }
            break;
        case FADE_CIRCLE_LEFT:
            if (calc_x > (screenWidth / 2)) {
                if (calc_y > (screenHeight / 2)) {
                    obj->transition_applied = FADE_UP_STATIONARY;
                } else {
                    obj->transition_applied = FADE_DOWN_STATIONARY;
                }
            } else {
                if (calc_y > (screenHeight / 2)) {
                    obj->transition_applied = FADE_UP_SLOW;
                } else {
                    obj->transition_applied = FADE_DOWN_SLOW;
                }
            }
            break;
        case FADE_CIRCLE_RIGHT:
            if (calc_x > (screenWidth / 2)) {
                if (calc_y > (screenHeight / 2)) {
                    obj->transition_applied = FADE_UP_SLOW;
                } else {
                    obj->transition_applied = FADE_DOWN_SLOW;
                }
            } else {
                if (calc_y > (screenHeight / 2)) {
                    obj->transition_applied = FADE_UP_STATIONARY;
                } else {
                    obj->transition_applied = FADE_DOWN_STATIONARY;
                }
            }
            break;
        default:
            obj->transition_applied = current_fading_effect;  
    }   
}

int layersDrawn = 0;

int compare_by_layer_index(const void *a, const void *b) {
    GDLayerSortable *la = *(GDLayerSortable **)a;
    GDLayerSortable *lb = *(GDLayerSortable **)b;

    int layerA = la->layer->layerNum;
    int layerB = lb->layer->layerNum;

    if (layerA != layerB) {
        return layerA - layerB;
    } 

    return la->originalIndex - lb->originalIndex;
}

void draw_all_object_layers() {
    u64 t0 = gettime();
    if (GRRLIB_Settings.antialias == false) {
        GX_SetCopyFilter(GX_FALSE, rmode->sample_pattern, GX_FALSE, rmode->vfilter);
    } else {
        GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    }
    
    float screen_x_max = screenWidth + 90.0f;
    float screen_y_max = screenHeight + 90.0f;

    int cam_sx = (int)((state.camera_x + SCREEN_WIDTH_AREA / 2) / GFX_SECTION_SIZE);
    int cam_sy = (int)((state.camera_y + SCREEN_HEIGHT_AREA / 2) / GFX_SECTION_SIZE);

    GDLayerSortable *visible_layers[MAX_VISIBLE_LAYERS];
    int visible_count = 1;

    visible_layers[0] = &gfx_player_layer;

    int width = (SCREEN_WIDTH_AREA / 2) / GFX_SECTION_SIZE + 2;
    int height = (SCREEN_HEIGHT_AREA / 2) / GFX_SECTION_SIZE + 2;

    // Gather layers from all visible sections
    for (int dx = -width; dx <= width; dx++) {
        for (int dy = -height; dy <= height; dy++) {
            GFXSection *sec = get_or_create_gfx_section(cam_sx + dx, cam_sy + dy);
            for (int i = 0; i < sec->layer_count; i++) {
                GameObject *obj = sec->layers[i]->layer->obj;
                
                float calc_x = ((obj->x - state.camera_x) * SCALE) - widthAdjust;
                float calc_y = screenHeight - ((obj->y - state.camera_y) * SCALE);  
                if (calc_x > -90 && calc_x < screen_x_max) {        
                    if (calc_y > -90 && calc_y < screen_y_max) {    
                        if (visible_count < MAX_VISIBLE_LAYERS) {
                            // Add to visible layers, as it can be seen
                            visible_layers[visible_count++] = sec->layers[i];
                        }
                    }
                }
            }
        }
    }

    // Sort layers
    qsort(visible_layers, visible_count, sizeof(GDLayerSortable*), compare_sortable_layers);


    // Sort same ID objects so instead of layer 0 layer 1 layer 0 layer 1, its layer 0, layer 0, layer 1, layer 1
    int i = 0;
    while (i < visible_count) {
        int obj_id = visible_layers[i]->layer->obj->id;
        int j = i + 1;

        // Find run of same object
        while (j < visible_count &&
            visible_layers[j]->layer->obj->id == obj_id) {
            j++;
        }

        // Reorder [i, j) to group layers by increasing layer index
        if (j - i > 1) {
            qsort(&visible_layers[i], j - i, sizeof(GDLayerSortable*), compare_by_layer_index);
        }

        i = j;
    }
    
    u64 t1 = gettime();
    layer_sorting = ticks_to_microsecs(t1 - t0) / 1000.f;
    
    draw_particles(GLITTER_EFFECT);
    layersDrawn = visible_count;

    prev_blending = -1;

    // Draw in sorted order
    for (int i = 0; i < visible_count; i++) {
        GDObjectLayer *layer = visible_layers[i]->layer;
        GameObject *obj = layer->obj;

        int obj_id = obj->id;

        if (obj_id == PLAYER_OBJECT) {
            // Draw player related stuff
            u64 t2 = gettime();
            draw_particles(DEATH_CIRCLE);
            draw_particles(DEATH_PARTICLES);
            draw_particles(CUBE_DRAG);
            draw_particles(SHIP_TRAIL);
            draw_particles(HOLDING_SHIP_TRAIL);
            draw_particles(UFO_JUMP);
            draw_particles(UFO_TRAIL);
            draw_particles(DUAL_BALL_HITBOX_EFFECT);
            draw_particles(P1_TRAIL);
            
            if (death_timer <= 0) {
                trail = trail_p1;
                wave_trail = wave_trail_p1;
                state.current_player = 0;
                draw_player(&state.player);
                trail_p1 = trail;
                wave_trail_p1 = wave_trail;

                if (state.dual) {
                    Color temp = p1;
                    p1 = p2;
                    p2 = temp;
                    trail = trail_p2;
                    wave_trail = wave_trail_p2;
                    state.current_player = 1;
                    draw_player(&state.player2);
                    trail_p2 = trail;
                    wave_trail_p2 = wave_trail;
                    temp = p1;
                    p1 = p2;
                    p2 = temp;
                }
            }
            draw_particles(SHIP_DRAG);
            draw_particles(COIN_OBJ);
            u64 t3 = gettime();
            player_draw_time = ticks_to_microsecs(t3 - t2) / 1000.f;

            // Restore variables
            set_texture(prev_tex);
            GRRLIB_SetBlend(prev_blending);
        } else if (obj_id < OBJECT_COUNT) {
            u64 t0 = gettime();
            float calc_x = ((obj->x - state.camera_x) * SCALE) - widthAdjust;
            float calc_y = screenHeight - ((obj->y - state.camera_y) * SCALE);  

            int fade_val = get_fade_value(calc_x, screenWidth);
            bool fade_edge = (fade_val == 255 || fade_val == 0);
            bool is_layer0 = (layer->layerNum == 0);

            // Fade objects
            if (is_layer0 && fade_edge) handle_special_fading(obj, calc_x, calc_y);
            
            // If saw, rotate
            if (is_layer0 && objects[obj_id].is_saw && !state.paused) {
                obj->rotation += ((obj->random & 1) ? -get_rotation_speed(obj) : get_rotation_speed(obj)) * dt;
            }

            handle_pre_draw_object_particles(obj, layer); 
            u64 t1 = gettime();
            obj_particles_time += t1 - t0;

            t0 = gettime();
            if (!obj->toggled) put_object_layer(obj, calc_x, calc_y, layer);
            t1 = gettime();
            draw_time += t1 - t0;
            
            t0 = gettime();
            handle_post_draw_object_particles(obj, layer); 
            t1 = gettime();
            obj_particles_time += t1 - t0;
        }
    }

    draw_particles(SPEEDUP);
    
    prev_tex = NULL;
    prev_blending = GRRLIB_BLEND_ALPHA;
    GRRLIB_SetBlend(GRRLIB_BLEND_ALPHA);

    if (state.hitbox_display) { 
        GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);
        GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
        GX_SetVtxDesc(GX_VA_TEX0,   GX_NONE);
        for (int dx = -width; dx <= width; dx++) {
            for (int dy = -height; dy <= height; dy++) {
                GFXSection *sec = get_or_create_gfx_section(cam_sx + dx, cam_sy + dy);
                for (int i = 0; i < sec->layer_count; i++) {
                    GameObject *obj = sec->layers[i]->layer->obj;
                    
                    float calc_x = ((obj->x - state.camera_x) * SCALE) - widthAdjust;
                    float calc_y = screenHeight - ((obj->y - state.camera_y) * SCALE);  
                    if (calc_x > -90 && calc_x < screen_x_max) {        
                        if (calc_y > -90 && calc_y < screen_y_max) {    
                            if (sec->layers[i]->layerNum == 0) draw_hitbox(obj);
                        }
                    }
                }
            }
        }

        if (state.hitbox_display == 2) draw_hitbox_trail(0);
        draw_player_hitbox(&state.player);

        if (state.dual) {
            if (state.hitbox_display == 2) draw_hitbox_trail(1);
            draw_player_hitbox(&state.player2);
        }
        
        GX_SetVtxDesc(GX_VA_TEX0,   GX_DIRECT);
        GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    }

    draw_time = ticks_to_microsecs(draw_time) / 1000.f;
    obj_particles_time = ticks_to_microsecs(obj_particles_time) / 1000.f;
    
    
    GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);
}

void handle_col_triggers() {
    for (int chan = 0; chan < COL_CHANNEL_COUNT; chan++) {
        struct ColTriggerBuffer *buffer = &col_trigger_buffer[chan];

        if (buffer->active) {
            Color lerped_color;
            Color color_to_lerp = buffer->new_color;
            float alpha_to_lerp = buffer->new_alpha;
            float lerped_alpha;

            if (buffer->copy_channel_id) {
                color_to_lerp = channels[buffer->copy_channel_id].color;
                alpha_to_lerp = channels[buffer->copy_channel_id].alpha;
            }

            if (buffer->seconds > 0) {
                float multiplier = buffer->time_run / buffer->seconds;
                lerped_color = color_lerp(buffer->old_color, color_to_lerp, multiplier);
                lerped_alpha = (alpha_to_lerp - buffer->old_alpha) * multiplier + buffer->old_alpha;
            } else {
                lerped_color = color_to_lerp;
                lerped_alpha = alpha_to_lerp;
            }

            channels[chan].color = lerped_color;
            channels[chan].alpha = lerped_alpha;

            buffer->time_run += STEPS_DT;

            if (buffer->time_run > buffer->seconds) {
                buffer->active = FALSE;
                if (buffer->copy_channel_id) {
                    channels[chan].hsv = buffer->copy_channel_HSV;
                    channels[chan].copy_color_id = buffer->copy_channel_id;
                } else {
                    channels[chan].copy_color_id = 0;
                }
                channels[chan].color = color_to_lerp;
                channels[chan].alpha = alpha_to_lerp;
            }
        }
    }
}

void handle_copy_channels() {
    for (int chan = 0; chan < COL_CHANNEL_COUNT; chan++) {
        int copy_color_id = channels[chan].copy_color_id;
        if (copy_color_id > 0) {
            Color color = channels[copy_color_id].color;
            color = HSV_combine(color, channels[chan].hsv);
            channels[chan].color = color;
        }
    }
}

void upload_to_buffer(GameObject *obj, int channel) {
    if (channel == 0) channel = 1;
    struct ColTriggerBuffer *buffer = &col_trigger_buffer[channel];
    buffer->active = TRUE;
    buffer->old_color = channels[channel].color;
    buffer->old_alpha = channels[channel].alpha;
    if (obj->trigger.col_trigger.p1_color) {
        buffer->new_color.r = p1.r;
        buffer->new_color.g = p1.g;
        buffer->new_color.b = p1.b;
        buffer->new_alpha = 1.f;
    } else if (obj->trigger.col_trigger.p2_color) {
        buffer->new_color.r = p2.r;
        buffer->new_color.g = p2.g;
        buffer->new_color.b = p2.b;
        buffer->new_alpha = 1.f;
    } else {
        buffer->new_color.r = obj->trigger.col_trigger.trig_colorR;
        buffer->new_color.g = obj->trigger.col_trigger.trig_colorG;
        buffer->new_color.b = obj->trigger.col_trigger.trig_colorB;
        buffer->new_alpha   = obj->trigger.col_trigger.opacity;
    }

    int copy_color_id = obj->trigger.col_trigger.copied_color_id;
    if (copy_color_id > 0) {
        buffer->copy_channel_HSV = obj->trigger.col_trigger.copied_hsv;
        buffer->copy_channel_id = copy_color_id;
    } else {
        buffer->copy_channel_id = 0;
    }

    if (channel < BG) {
        channels[channel].blending = obj->trigger.col_trigger.blending;
    }
    buffer->seconds = obj->trigger.trig_duration;
    buffer->time_run = 0;
}

int convert_ease(int easing) {
    switch (easing) {
        case 0: return EASE_LINEAR;

        case 1: return EASE_IN_OUT;
        case 2: return EASE_IN;
        case 3: return EASE_OUT;
        
        case 4: return ELASTIC_IN_OUT;
        case 5: return ELASTIC_IN;
        case 6: return ELASTIC_OUT;
        
        case 7: return BOUNCE_IN_OUT;
        case 8: return BOUNCE_IN;
        case 9: return BOUNCE_OUT;
        
        case 10: return EXPO_IN_OUT;
        case 11: return EXPO_IN;
        case 12: return EXPO_OUT;
        
        case 13: return SINE_IN_OUT;
        case 14: return SINE_IN;
        case 15: return SINE_OUT;
        
        case 16: return BACK_IN_OUT;
        case 17: return BACK_IN;
        case 18: return BACK_OUT;
    }
    return EASE_LINEAR;
}

void handle_move_triggers() {
    // First reset deltas
    for (int slot = 0; slot < MAX_MOVING_CHANNELS; slot++) {
        struct MoveTriggerBuffer *buffer = &move_trigger_buffer[slot];

        if (buffer->active) {
            for (Node *p = get_group(buffer->target_group); p; p = p->next) {
                p->obj->object.delta_x = 0;
                p->obj->object.delta_y = 0;   
            }
        }
    }

    for (int slot = 0; slot < MAX_MOVING_CHANNELS; slot++) {
        struct MoveTriggerBuffer *buffer = &move_trigger_buffer[slot];

        if (buffer->active) {
            // Iterate through group objects
            int i = 0;
            
            // Calculate delta
            float delta_x;
            float delta_y;
            if (buffer->lock_to_player_x) {    
                delta_x = state.player.vel_x * STEPS_DT;
            } else {
                float before_x = buffer->move_last_x;
                float after_x = buffer->offsetX * easeTime(convert_ease(buffer->easing), buffer->time_run, buffer->seconds, 2.0f);
                
                delta_x = after_x - before_x;
                buffer->move_last_x = after_x;
            } 
            
            if (buffer->lock_to_player_y) {
                delta_y = state.player.vel_y * STEPS_DT;;
            } else {
                float before_y = buffer->move_last_y;
                float after_y = buffer->offsetY * easeTime(convert_ease(buffer->easing), buffer->time_run, buffer->seconds, 2.0f);

                delta_y = after_y - before_y;
                buffer->move_last_y = after_y;
            }

            for (Node *p = get_group(buffer->target_group); p; p = p->next) {
                GameObject *obj = p->obj;
                
                float new_x = obj->x + delta_x;
                float new_y = obj->y + delta_y;

                obj->object.delta_x += delta_x / STEPS_DT;
                obj->object.delta_y += delta_y / STEPS_DT;

                update_object_section(obj, new_x, new_y);

                if (obj->object.touching_player) {
                    Player *player;
                    if (obj->object.touching_player == 1) {
                        player = &state.player;
                    } else if (obj->object.touching_player >= 2) {
                        player = &state.player2;
                    }

                    float grav_delta_y = grav(player, delta_y / STEPS_DT);
                    if (grav_delta_y >= -MOVE_SPEED_DIVIDER) {
                        player->y += delta_y;
                    }
                } else if (obj->object.prev_touching_player) {
                    // Exiting
                    Player *player;
                    if (obj->object.prev_touching_player == 1) {
                        player = &state.player;
                    } else if (obj->object.prev_touching_player >= 2) {
                        player = &state.player2;
                    }

                    float grav_delta_y = grav(player, delta_y / STEPS_DT);
                    if (grav_delta_y < -MOVE_SPEED_DIVIDER) {
                        // Nothing
                    } else if (grav_delta_y <= 0) {
                        // Nothing
                    } else if (grav_delta_y <= MOVE_SPEED_DIVIDER) {
                        // Nothing
                    } else {
                        player->vel_y = grav_delta_y;
                    }
                }

                i++;
            }
            buffer->time_run += STEPS_DT;

            if (buffer->time_run > buffer->seconds) {
                buffer->active = FALSE;
                
                // Clear deltas
                for (Node *p = get_group(buffer->target_group); p; p = p->next) {
                    GameObject *obj = p->obj;
                    
                    float delta_y = p->obj->object.delta_y * STEPS_DT;
                    if (obj->object.touching_player) {
                        Player *player;
                        if (obj->object.touching_player == 1) {
                            player = &state.player;
                        } else if (obj->object.touching_player >= 2) {
                            player = &state.player2;
                        } else {
                            player = &state.player;
                        }

                        float grav_delta_y = grav(player, delta_y / STEPS_DT);
                        if (grav_delta_y < -MOVE_SPEED_DIVIDER) {
                            // Nothing
                        } else if (grav_delta_y <= 0) {
                            // Nothing
                        } else if (grav_delta_y <= MOVE_SPEED_DIVIDER) {
                            // Nothing
                        } else {
                            player->vel_y = grav_delta_y;
                        }
                    }
                    
                    obj->object.delta_x = 0;
                    obj->object.delta_y = 0;   
                }
            }
        }
    }
}

int obtain_free_alpha_slot() {
    for (int i = 0; i < MAX_ALPHA_CHANNELS; i++) {
        if (!alpha_trigger_buffer[i].active) return i;
    }
    return -1;
}

void upload_to_alpha_buffer(GameObject *obj) {
    int slot = obtain_free_alpha_slot();
    if (slot >= 0) {
        struct AlphaTriggerBuffer *buffer = &alpha_trigger_buffer[slot];

        buffer->new_alpha = obj->trigger.alpha_trigger.opacity;
        buffer->target_group = obj->trigger.alpha_trigger.target_group;

        buffer->time_run = 0;
        buffer->seconds = obj->trigger.trig_duration;
        
        int count = 0;

        float capacity = 8;
        
        float *opacities = malloc(sizeof(float) * capacity); // Initial 8

        for (Node *p = get_group(buffer->target_group); p; p = p->next) {
            if (count >= capacity) {
                capacity *= 2; // grow exponentially
                float *tmp = realloc(opacities, sizeof(float) * capacity);
                if (!tmp) {
                    free(opacities);
                    printf("Couldn't allocate alpha values\n");
                    return;
                }
                opacities = tmp;
            }

            GameObject *obj = p->obj;
            opacities[count++] = obj->opacity;
        }
        buffer->initial_opacities = opacities;
        
        buffer->active = TRUE;
    }
}

void handle_alpha_triggers() {
    for (int slot = 0; slot < MAX_ALPHA_CHANNELS; slot++) {
        struct AlphaTriggerBuffer *buffer = &alpha_trigger_buffer[slot];
        
        if (buffer->active) {
            int i = 0;
            for (Node *p = get_group(buffer->target_group); p; p = p->next) {
                GameObject *obj = p->obj;
                float lerped_alpha;

                if (buffer->seconds > 0) {
                    float multiplier = buffer->time_run / buffer->seconds;
                    lerped_alpha = (buffer->new_alpha - buffer->initial_opacities[i]) * multiplier + buffer->initial_opacities[i];
                } else {
                    lerped_alpha = buffer->new_alpha;
                }

                obj->opacity = lerped_alpha;

                i++;
            }
            buffer->time_run += STEPS_DT;

            if (buffer->time_run > buffer->seconds) {
                buffer->active = FALSE;

                // Set new alpha on all objects
                for (Node *p = get_group(buffer->target_group); p; p = p->next) {
                    GameObject *obj = p->obj;
                    obj->opacity = buffer->new_alpha;
                }

                free(buffer->initial_opacities);
            }
        }
    }
}


int obtain_free_move_slot() {
    for (int i = 0; i < MAX_MOVING_CHANNELS; i++) {
        if (!move_trigger_buffer[i].active) return i;
    }
    return -1;
}

void upload_to_move_buffer(GameObject *obj) {
    int slot = obtain_free_move_slot();
    if (slot >= 0) {
        struct MoveTriggerBuffer *buffer = &move_trigger_buffer[slot];

        buffer->easing = obj->trigger.move_trigger.easing;

        buffer->offsetX = obj->trigger.move_trigger.offsetX;
        buffer->offsetY = obj->trigger.move_trigger.offsetY;

        buffer->lock_to_player_x = obj->trigger.move_trigger.lock_to_player_x;
        buffer->lock_to_player_y = obj->trigger.move_trigger.lock_to_player_y;

        buffer->target_group = obj->trigger.move_trigger.target_group;

        buffer->time_run = 0;
        buffer->seconds = obj->trigger.trig_duration;

        buffer->move_last_x = 0;
        buffer->move_last_y = 0;
        
        buffer->active = TRUE;
    }
}

void run_trigger(GameObject *obj) {
    switch (obj->id) {
        case TRIGGER_FADE_NONE:
            current_fading_effect = FADE_NONE;
            break;
            
        case TRIGGER_FADE_UP:
            current_fading_effect = FADE_UP;
            break;
            
        case TRIGGER_FADE_DOWN:
            current_fading_effect = FADE_DOWN;
            break;
            
        case TRIGGER_FADE_RIGHT:
            current_fading_effect = FADE_RIGHT;
            break;
            
        case TRIGGER_FADE_LEFT:
            current_fading_effect = FADE_LEFT;
            break;
            
        case TRIGGER_FADE_SCALE_IN:
            current_fading_effect = FADE_SCALE_IN;
            break;
            
        case TRIGGER_FADE_SCALE_OUT:
            current_fading_effect = FADE_SCALE_OUT;
            break;
        
        case TRIGGER_FADE_INWARDS:
            current_fading_effect = FADE_INWARDS;
            break;

        case TRIGGER_FADE_OUTWARDS:
            current_fading_effect = FADE_OUTWARDS;
            break;
        
        case TRIGGER_FADE_LEFT_SEMICIRCLE:
            current_fading_effect = FADE_CIRCLE_LEFT;
            break;

        case TRIGGER_FADE_RIGHT_SEMICIRCLE:
            current_fading_effect = FADE_CIRCLE_RIGHT;
            break;

        case BG_TRIGGER:
            upload_to_buffer(obj, BG);
            if (!obj->trigger.col_trigger.tintGround) break;
        
        case GROUND_TRIGGER:
            upload_to_buffer(obj, GROUND);
            break;
                    
        case LINE_TRIGGER:
        case 915: // gd converts 1.4 line trigger to 2.0 one for some reason
            upload_to_buffer(obj, LINE);
            break;
        
        case OBJ_TRIGGER:
            upload_to_buffer(obj, OBJ);
            break;
        
        case OBJ_2_TRIGGER:
            upload_to_buffer(obj, 1);
            break;
        
        case COL2_TRIGGER: // col 2
            upload_to_buffer(obj, 2);
            break;

        case COL3_TRIGGER: // col 3
            upload_to_buffer(obj, 3);
            break;
            
        case COL4_TRIGGER: // col 4
            upload_to_buffer(obj, 4);
            break;
            
        case THREEDL_TRIGGER: // 3DL
            upload_to_buffer(obj, THREEDL);
            break;

        case ENABLE_TRAIL:
            p1_trail = TRUE;
            break;
        
        case DISABLE_TRAIL:
            p1_trail = FALSE;
            break;

        case 899: // 2.0 color trigger
            upload_to_buffer(obj, obj->trigger.col_trigger.target_color_id);
            break;
        
        case 901: // Move trigger
            upload_to_move_buffer(obj);
            break;

        case 1007: // Alpha trigger
            upload_to_alpha_buffer(obj);
            break;
    }
    obj->activated[0] = TRUE;
}

void handle_triggers(GameObject *obj) {
    int obj_id = obj->id;
    Player *player = &state.player;
    
    if ((objects[obj_id].is_trigger || obj->id > OBJECT_COUNT) && !obj->activated[0]) {
        if (obj->trigger.touchTriggered) {
            if (intersect(
                player->x, player->y, player->width, player->height, 0, 
                obj->x, obj->y, 30, 30, obj->rotation
            )) {
                run_trigger(obj);
            }
        } else {
            if (obj->x < state.player.x) {
                run_trigger(obj);
            }
        }
    }
}

u64 last_beat_time = 0;
int pulse_frames_left = 0;
int beat_pulse = 0;

void update_beat() {
    const float beat_interval_ms = 60000.f / songs[level_info.song_id].tempo;

    u64 now = gettime();
    u64 elapsed_ms = ticks_to_millisecs(now - last_beat_time);

    // Check if we've hit the next beat
    if (elapsed_ms >= beat_interval_ms) {
        last_beat_time = now;
        pulse_frames_left = 20;
        beat_pulse = true;
    }

    // Countdown pulse frames
    if (pulse_frames_left > 0) {
        pulse_frames_left--;
        beat_pulse = true;
    } else {
        beat_pulse = false;
    }

    if (MP3Player_GetAmplitude() < 0.05f) {
        beat_pulse = false;
    }
}

void handle_objects() {
    int sx = (int)(state.player.x / SECTION_SIZE);
    for (int dx = -1; dx <= 1; dx++) {
        for (int sy = -400; sy <= MAX_LEVEL_HEIGHT / SECTION_SIZE; sy++) {
            Section *sec = get_or_create_section(sx + dx, sy);
            for (int i = 0; i < sec->object_count; i++) {
                GameObject *obj = sec->objects[i];
                handle_triggers(obj);
            }
        }
    }
    calculate_lbg();
    handle_col_triggers();
    handle_move_triggers();
    handle_alpha_triggers();
    handle_copy_channels();
}