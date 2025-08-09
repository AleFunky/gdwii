#include "player.h"
#include "collision.h"
#include "level_loading.h"
#include "main.h"
#include <wiiuse/wpad.h>
#include <math.h>
#include "math.h"
#include "object_includes.h"
#include <unistd.h>
#include <stdio.h>
#include "icons_includes.h"
#include "game.h"
#include "custom_mp3player.h"
#include "trail.h"
#include "objects.h"
#include "oggplayer.h"
#include "explode_11_ogg.h"

GRRLIB_texImg *icon_l1;
GRRLIB_texImg *icon_l2;
GRRLIB_texImg *ship_l1;
GRRLIB_texImg *ship_l2;
GRRLIB_texImg *ball_l1;
GRRLIB_texImg *ball_l2;
GRRLIB_texImg *ufo_l1;
GRRLIB_texImg *ufo_l2;
GRRLIB_texImg *ufo_dome;
GRRLIB_texImg *wave_l1;
GRRLIB_texImg *wave_l2;

GRRLIB_texImg *trail_tex;
GRRLIB_texImg *wave_trail_tex;

MotionTrail trail;
MotionTrail trail_p1;
MotionTrail trail_p2;

MotionTrail wave_trail;
MotionTrail wave_trail_p1;
MotionTrail wave_trail_p2;

Color p1;
Color p2;

inline static float getTop(Player *player)  { return player->y + player->height / 2; }
inline static float getBottom(Player *player)  { return player->y - player->height / 2; }

inline static float getGroundTop(Player *player)  { return player->y + (player->height / 2) + ((player->gamemode == GAMEMODE_WAVE) ? (player->mini ? 3 : 5) : 0); }
inline static float getGroundBottom(Player *player)  { return player->y - (player->height / 2) - ((player->gamemode == GAMEMODE_WAVE) ? (player->mini ? 3 : 5) : 0); }

inline static float getRight(Player *player)  { return player->x + player->width / 2; }
inline static float getLeft(Player *player)  { return player->x - player->width / 2; }

inline static float getInternalTop(Player *player)  { return player->y + player->internal_hitbox.height / 2; }
inline static float getInternalBottom(Player *player)  { return player->y - player->internal_hitbox.height / 2; }
inline static float getInternalRight(Player *player)  { return player->x + player->internal_hitbox.width / 2; }
inline static float getInternalLeft(Player *player)  { return player->x - player->internal_hitbox.width / 2; }

inline static float gravBottom(Player *player) { return player->upside_down ? -getTop(player) : getBottom(player); }
inline static float gravTop(Player *player) { return player->upside_down ? -getBottom(player) : getTop(player); }

inline static float gravInternalBottom(Player *player) { return player->upside_down ? -getInternalTop(player) : getInternalBottom(player); }
inline static float gravInternalTop(Player *player) { return player->upside_down ? -getInternalBottom(player) : getInternalTop(player); }

inline static float grav(Player *player, float val) { return player->upside_down ? -val : val; }

inline static float obj_getTop(GameObject *object)  { 
    return object->y + object->height / 2; 
}
inline static float obj_getBottom(GameObject *object)  { 
    return object->y - object->height / 2; 
}
inline static float obj_getRight(GameObject *object)  {  
    return object->x + object->width / 2; 
}
inline static float obj_getLeft(GameObject *object)  { 
    return object->x - object->width / 2; 
}
inline static float obj_gravBottom(Player *player, GameObject *object) { return player->upside_down ? -obj_getTop(object) : obj_getBottom(object); }
inline static float obj_gravTop(Player *player, GameObject *object) { return player->upside_down ? -obj_getBottom(object) : obj_getTop(object); }

const float player_speeds[SPEED_COUNT] = {
	251.16007972276924,
	311.580093712804,
	387.42014039710523,
	468.0001388338566
};

const float slopeHeights[SPEED_COUNT] = {
    322.345224,
    399.889818,
    497.224926,
    600.643296
};

const float cube_jump_heights[SPEED_COUNT] = {
    573.48,
    603.72,
    616.68,
    606.42,
};

void set_p_velocity(Player *player, float vel) {
    player->vel_y = vel * ((player->mini) ? 0.8 : 1);
}

void handle_collision(Player *player, GameObject *obj, ObjectHitbox *hitbox) {
    InternalHitbox internal = player->internal_hitbox;

    int clip = (player->gamemode == GAMEMODE_SHIP || player->gamemode == GAMEMODE_UFO) ? 7 : 10;
    switch (hitbox->type) {
        case HITBOX_BREAKABLE_BLOCK:
        case HITBOX_SOLID: 
            bool gravSnap = FALSE;

            clip += fabsf(player->vel_y) * dt;
            
            float bottom = gravBottom(player);
            if (player->slope_data.slope) {
                bottom = bottom + sinf(slope_angle(player->slope_data.slope, player)) * player->height / 2;
                clip = 7;
                if (obj_gravTop(player, obj) - bottom < 2)
                    return;
            }
            
            if (objects[obj->id].is_slope) {
                slope_collide(obj, player);
                break;
            }


            if (player->gravObj && player->gravObj->hitbox_counter[state.current_player] == 1) {
                // Only do the funny grav snap if player is touching a gravity object and internal hitbox is touching block
                bool internalCollidingBlock = intersect(
                    player->x, player->y, internal.width, internal.height, 0, 
                    obj->x, obj->y, hitbox->width, hitbox->height, obj->rotation
                );

                gravSnap = (!state.old_player.on_ground || player->ceiling_inv_time > 0) && internalCollidingBlock && obj_gravTop(player, obj) - gravInternalBottom(player) <= clip;
            }

            bool slope_condition = player->touching_slope && !player_circle_touches_slope(player->potentialSlope, player) && !state.old_player.on_ground && !state.old_player.on_ceiling && obj_getRight(player->potentialSlope) - getLeft(player) < 6 && (player->potentialSlope->orientation == 1 || player->potentialSlope->orientation == 2);
            
            bool safeZone = (obj_gravTop(player, obj) - gravBottom(player) <= clip) || (gravTop(player) - obj_gravBottom(player, obj) <= clip);
            
            if ((player->gamemode == GAMEMODE_WAVE || (!gravSnap && !safeZone)) && intersect(
                player->x, player->y, internal.width, internal.height, 0, 
                obj->x, obj->y, hitbox->width, hitbox->height, obj->rotation
            )) {
                if (hitbox->type == HITBOX_BREAKABLE_BLOCK) {
                    obj->toggled = TRUE;
                    for (s32 i = 0; i < 10; i++) {
                        spawn_particle(BREAKABLE_BRICK_PARTICLES, obj->x, obj->y, obj);
                    }
                } else {
                    state.dead = TRUE;
                }
            } else if (obj_gravTop(player, obj) - gravBottom(player) <= clip && player->vel_y <= 0 && !slope_condition && player->gamemode != GAMEMODE_WAVE) {
                player->y = grav(player, obj_gravTop(player, obj)) + grav(player, player->height / 2);
                player->vel_y = 0;
                player->on_ground = TRUE;
                player->inverse_rotation = FALSE;
                player->time_since_ground = 0;
            } else if (player->gamemode != GAMEMODE_WAVE) {
                // Ufo can break breakable blocks from above, so dont use as a ceiling
                if (player->gamemode == GAMEMODE_UFO && hitbox->type == HITBOX_BREAKABLE_BLOCK) {
                    break;
                }
                // Behave normally
                if (player->gamemode != GAMEMODE_CUBE || gravSnap) {
                    if (((gravTop(player) - obj_gravBottom(player, obj) <= clip && player->vel_y > 0) || gravSnap) && !slope_condition) {
                        player->vel_y = 0;
                        if (!gravSnap) player->on_ceiling = TRUE;
                        player->inverse_rotation = FALSE;
                        player->time_since_ground = 0;
                        player->y = grav(player, obj_gravBottom(player, obj)) - grav(player, player->height / 2);
                    }
                }
            }
            break;
        case HITBOX_SPIKE:
            state.dead = TRUE;
            break;
        case HITBOX_SPECIAL:
            handle_special_hitbox(player, obj, hitbox);
            break;
    }
}

float player_get_vel(Player *player, float vel) {
    return vel * (player->upside_down ? -1 : 1);
}

void collide_with_obj(Player *player, GameObject *obj) {
    ObjectHitbox *hitbox = (ObjectHitbox *) &objects[obj->id].hitbox;

    if (hitbox->type != HITBOX_NONE && !obj->toggled && obj->id < OBJECT_COUNT) {
        number_of_collisions_checks++;
        if (hitbox->is_circular) {
            if (intersect_rect_circle(
                player->x, player->y, player->width, player->height, player->rotation, 
                obj->x, obj->y, hitbox->radius
            )) {
                handle_collision(player, obj, hitbox);
                obj->collided[state.current_player] = TRUE;
                number_of_collisions++;
            } else {
                obj->collided[state.current_player] = FALSE;
            }
        } else {
            float obj_rot = normalize_angle(obj->rotation);
            float rotation = (obj_rot == 0 || obj_rot == 90 || obj_rot == 180 || obj_rot == 270) ? 0 : player->rotation;
            
            bool checkColl = intersect(
                player->x, player->y, player->width, player->height, rotation, 
                obj->x, obj->y, hitbox->width, hitbox->height, obj_rot
            );
            
            // Rotated hitboxes must also collide with the unrotated hitbox
            if (rotation != 0) {
                checkColl = checkColl && intersect(
                    player->x, player->y, player->width, player->height, 0, 
                    obj->x, obj->y, hitbox->width, hitbox->height, obj_rot
                );
            }

            if (checkColl) {
                handle_collision(player, obj, hitbox);
                obj->collided[state.current_player] = TRUE;
                number_of_collisions++;
            } else {
                obj->collided[state.current_player] = FALSE;
            }
            
        }
    } else {
        obj->collided[state.current_player] = FALSE;
    }
}

GameObject *slope_buffer[MAX_COLLIDED_OBJECTS];
int slope_count = 0;

GameObject *block_buffer[MAX_COLLIDED_OBJECTS];
int block_count = 0;

GameObject *hazard_buffer[MAX_COLLIDED_OBJECTS];
int hazard_count = 0;

void collide_with_objects(Player *player) {
    number_of_collisions = 0;
    number_of_collisions_checks = 0;

    int sx = (int)(player->x / SECTION_SIZE);
    int sy = (int)(player->y / SECTION_SIZE);
    
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            Section *sec = get_or_create_section(sx + dx, sy + dy);
            for (int i = 0; i < sec->object_count; i++) {
                GameObject *obj = sec->objects[i];
                ObjectHitbox *hitbox = (ObjectHitbox *) &objects[obj->id].hitbox;
                
                if (hitbox->type == HITBOX_SOLID) {
                    if (objects[obj->id].is_slope) {
                        slope_buffer[slope_count++] = obj;
                    } else {
                        block_buffer[block_count++] = obj;
                    }
                } else if (hitbox->type == HITBOX_SPIKE) {
                    hazard_buffer[hazard_count++] = obj;
                } else { // HITBOX_SPECIAL
                    collide_with_obj(player, obj);
                }
            }
        }
    }

    if (player->left_ground) {
        clear_slope_data(player);
    }

    float closestDist = 999999.f;
    // Detect if touching slope
    for (int i = 0; i < slope_count; i++) {
        GameObject *obj = slope_buffer[i];
        if (intersect(
            player->x, player->y, player->width, player->height, 0, 
            obj->x, obj->y, obj->width, obj->height, obj->rotation
        )) {
            float dist = square_distance(player->x, player->y, obj->x, obj->y);
            if (dist < closestDist) {
                player->touching_slope = TRUE;
                player->potentialSlope = obj;
                closestDist = dist; 
            }
            break;
        }
    }

    for (int i = 0; i < block_count; i++) {
        GameObject *obj = block_buffer[i];
        collide_with_obj(player, obj);
    }

    for (int i = 0; i < slope_count; i++) {
        GameObject *obj = slope_buffer[i];
        collide_with_obj(player, obj);
    }
    
    for (int i = 0; i < hazard_count; i++) {
        GameObject *obj = hazard_buffer[i];
        collide_with_obj(player, obj);
    }

    player->touching_slope = FALSE;
    slope_count = 0;
    block_count = 0;
    hazard_count = 0;
}

void cube_gamemode(Player *player) {
    int mult = (player->upside_down ? -1 : 1);
    
    trail.positionR = (Vec2){player->x, player->y};  
    trail.startingPositionInitialized = TRUE;

    player->gravity = -2794.1082;
    
    if (player->vel_y < -810) player->vel_y = -810;

    if (player->y > 2794.f) state.dead = TRUE;

    if (player->inverse_rotation) {
        player->rotation -= (415.3848f / 2) * STEPS_DT * mult * (player->mini ? 1.2f : 1.f);
    } else {
        player->rotation += 415.3848f * STEPS_DT * mult * (player->mini ? 1.2f : 1.f);
    }
    

    if (player->on_ground) {
        MotionTrail_StopStroke(&trail);
        if (!player->slope_data.slope) player->rotation = round(player->rotation / 90.0f) * 90.0f;
    }

    if ((player->time_since_ground < 0.05f) && (frame_counter & 0b11) == 0) {
        particle_templates[CUBE_DRAG].angle = (player->upside_down ? -90 : 90);
        particle_templates[CUBE_DRAG].gravity_y = (player->upside_down ? 300 : -300);
        spawn_particle(CUBE_DRAG, getLeft(player) + 4, (player->upside_down ? getTop(player) - 2 : getBottom(player) + 2), NULL);
    }

    if ((player->slope_data.slope || player->on_ground) && (state.input.holdA || state.input.hold2orY)) {
        if (player->slope_data.slope) {
            int orient = grav_slope_orient(player->slope_data.slope, player);
            if (orient == 0 || orient == 3) {
                float time = clampf(10 * (player->timeElapsed - player->slope_data.elapsed), 0.4f, 1.0f);
                set_p_velocity(player, 0.25f * time * slopeHeights[state.speed] + cube_jump_heights[state.speed]);
            } else {
                set_p_velocity(player, cube_jump_heights[state.speed]);
            }
        } else {
            set_p_velocity(player, cube_jump_heights[state.speed]);
        }

        player->on_ground = FALSE;
        player->buffering_state = BUFFER_END;

        if (!state.input.pressedA || !state.input.pressed2orY || !state.input.pressedB) {
            player->vel_y -= player->gravity * STEPS_DT;
            printf("Second jump\n");
        } else {
            printf("First jump\n");
        }
    }
}

void ship_particles(Player *player) {
    int mult = (player->upside_down ? -1 : 1);
    float scale = (player->mini) ? 0.6f : 1.f;
    
    float x, y;
    rotate_point_around_center(
        player->x, player->y,
        player->rotation,
        player->x - 12 * scale, player->y + (player->upside_down ? 10 : -10) * scale,
        &x, &y
    );
    
    trail.positionR = (Vec2){x, y};  
    trail.startingPositionInitialized = TRUE;

    if ((frame_counter & 0b11) == 0) {   
        // Particle trail
        spawn_particle(SHIP_TRAIL, x, y, NULL);
        
        // Holding particles
        if (state.input.holdA || state.input.hold2orY) {
            spawn_particle(HOLDING_SHIP_TRAIL, x, y, NULL);
        }

        // Ground drag effectr
        if (player->on_ground) {
            particle_templates[SHIP_DRAG].speed = 95 * mult;
            particle_templates[SHIP_DRAG].gravity_y = (player->upside_down ? 300 : -300);
            spawn_particle(SHIP_DRAG, player->x, (player->upside_down ? getTop(player) : getBottom(player)), NULL);
        }
    }
}

void update_ship_rotation(Player *player) {
    float diff_x = (player->x - state.old_player.x);
    float diff_y = (player->y - state.old_player.y);
    float angle_rad = atan2f(-diff_y, diff_x);
    player->rotation = iSlerp(player->rotation, RadToDeg(angle_rad), 0.05f, STEPS_DT);
}

void ship_gamemode(Player *player) {
    if (state.dual) {
        if (state.input.holdA || state.input.hold2orY) {
            player->buffering_state = BUFFER_END;
            if (player->vel_y <= 103.485492f)
                player->gravity = player->mini ? 1643.5872f : 1397.0491f;
            else
                player->gravity = player->mini ? 1314.86976f : 1117.64328f;
        } else {
            if (player->vel_y >= 103.485492f)
                player->gravity = player->mini ? -1577.85408f : -1341.1719f;
            else
                player->gravity = player->mini ? -1051.8984f : -894.11464f;
        }
    } else {
        if (state.input.holdA || state.input.hold2orY) {
            player->buffering_state = BUFFER_END;
            if (player->vel_y <= grav(player, 103.485492f))
                player->gravity = player->mini ? 1643.5872f : 1397.0491f;
            else
                player->gravity = player->mini ? 1314.86976f : 1117.64328f;
        } else {
            if (player->vel_y >= grav(player, 103.485492f))
                player->gravity = player->mini ? -1577.85408f : -1341.1719f;
            else
                player->gravity = player->mini ? -1051.8984f : -894.11464f;
        }
    }

    ship_particles(player);
    
    float min = player->mini ? -406.566f : -345.6f;
    float max = player->mini ? 508.248f : 432.0f;

    if (player->gravity < 0 && player->vel_y < min) {
        player->vel_y = min;
    } else if (player->gravity > 0 && player->vel_y > max) {
        player->vel_y = max;
    }
}

void ball_gamemode(Player *player) {
    int mult = (player->upside_down ? -1 : 1);
    
    trail.positionR = (Vec2){player->x, player->y};  
    trail.startingPositionInitialized = TRUE;

    player->gravity = -1676.46672f;  
    
    if (player->on_ground || player->on_ceiling) {
        player->ball_rotation_speed = 2.3;
        MotionTrail_StopStroke(&trail);

        if ((frame_counter & 0b11) == 0) {
            particle_templates[CUBE_DRAG].angle = (player->upside_down ? -90 : 90);
            particle_templates[CUBE_DRAG].gravity_y = (player->upside_down ? 300 : -300);
            spawn_particle(CUBE_DRAG, player->x, (player->upside_down ? getTop(player) - 2 : getBottom(player) + 2), NULL);
        }
    }

    // Jump
    if ((state.input.holdA || state.input.hold2orY) && (player->on_ground || player->on_ceiling || player->slope_data.slope) && player->buffering_state == BUFFER_READY) {
        player->upside_down ^= 1;
        set_p_velocity(player, -181.11601);

        player->buffering_state = BUFFER_END;
        
        player->ball_rotation_speed = -1.f;
    }
    
    player->rotation += player->ball_rotation_speed * mult * (player_speeds[state.speed] / player_speeds[SPEED_NORMAL]) / (player->mini ? 0.8 : 1);

    if (player->vel_y < -810) {
        player->vel_y = -810;
    } else if (player->vel_y > 810) {
        player->vel_y = 810;
    }
}

void ufo_particles(Player *player) {
    int mult = (player->upside_down ? -1 : 1);
    float scale = (player->mini) ? 0.6f : 1.f;
    
    float x, y;
    rotate_point_around_center(
        player->x, player->y,
        player->rotation,
        player->x, player->y + (player->upside_down ? 10 : -10) * scale,
        &x, &y
    );
    
    trail.positionR = (Vec2){x, y};  
    trail.startingPositionInitialized = TRUE;

    if ((frame_counter & 0b11) == 0) {   
        // Particle trail
        if ((frame_counter & 0b11) == 0) {
            spawn_particle(UFO_TRAIL, player->x, (player->upside_down ? getTop(player) - 4 : getBottom(player) + 4), NULL);
        }
        
        // Jump particles
        if (state.input.pressedA || state.input.pressed2orY || state.input.pressedB) {
            for (s32 i = 0; i < 5; i++) {
                spawn_particle(UFO_JUMP, x, y, NULL);
            }
        }

        // Ground drag effect
        if (player->on_ground) {
            particle_templates[SHIP_DRAG].speed = 95 * mult;
            particle_templates[SHIP_DRAG].gravity_y = (player->upside_down ? 100 : -300);
            spawn_particle(SHIP_DRAG, player->x, (player->upside_down ? getTop(player) : getBottom(player)), NULL);
        }
    }
}

void ufo_gamemode(Player *player) {
    int mult = (player->upside_down ? -1 : 1);

    if (player->buffering_state == BUFFER_READY && (state.input.pressedA || state.input.pressed2orY || state.input.pressedB || (state.old_player.gamemode == GAMEMODE_SHIP && (state.input.holdA || state.input.hold2orY || state.input.pressedB)))) {
        player->vel_y = maxf(player->vel_y, player->mini ? 358.992 : 371.034);
        player->buffering_state = BUFFER_END;
        player->ufo_last_y = player->y;
    } else {
        if (player->vel_y > 103.485492) {
            player->gravity = player->mini ? -1969.92 : -1671.84;
        } else {
            player->gravity = player->mini ? -1308.96 : -1114.56;
        }
    }

    ufo_particles(player);

    if (player->on_ground) {
        player->ufo_last_y = player->y;
    }

    if (!player->slope_data.slope) {
        float y_diff = (player->y - player->ufo_last_y) * mult;

        if (y_diff >= 0) {
            player->rotation = map_range(y_diff, 0.f, 60.f, 0.f, 10.f) * mult;
        } else {
            player->rotation = -map_range(-y_diff, 0.f, 300.f, 0.f, 25.f) * mult;
        }
    }

    float min = player->mini ? -406.566f : -345.6f;
    float max = player->mini ? 508.248f : 432.0f;

    if (player->vel_y < min) {
        player->vel_y = min;
    } else if (player->vel_y > max) {
        player->vel_y = max;
    }
}

void wave_gamemode(Player *player) {
    trail.positionR = (Vec2){player->x, player->y};  
    trail.startingPositionInitialized = TRUE;
    
    wave_trail.positionR = (Vec2){player->x, player->y};  
    wave_trail.startingPositionInitialized = TRUE;
    wave_trail.opacity = 1.f;

    if (player->buffering_state == BUFFER_READY) player->buffering_state = BUFFER_END;

    bool input = (state.input.holdA || state.input.hold2orY);
    player->gravity = 0;

    player->vel_y = (input * 2 - 1) * player_speeds[state.speed] * (player->mini ? 2 : 1);
    if (player->vel_y != state.old_player.vel_y || player->on_ground != state.old_player.on_ground || player->on_ceiling != state.old_player.on_ceiling) {
        MotionTrail_AddWavePoint(&wave_trail);
    }
}

void run_camera() {
    Player *player = &state.player;

    // Cap at camera_x
    if (state.camera_x + WIDTH_ADJUST_AREA + SCREEN_WIDTH_AREA >= level_info.wall_x) {
        state.camera_x = level_info.wall_x - (SCREEN_WIDTH_AREA + WIDTH_ADJUST_AREA);
    } else {
        state.camera_x += player->vel_x * STEPS_DT;
        state.ground_x += player->vel_x * STEPS_DT * state.mirror_speed_factor;
        state.background_x += player->vel_x * STEPS_DT * state.mirror_speed_factor;
    }

    float playable_height = state.ceiling_y - state.ground_y;
    float calc_height = 0;

    if (state.player.gamemode != GAMEMODE_CUBE || state.dual) {
        calc_height = (SCREEN_HEIGHT_AREA - playable_height) / 2;
    }

    state.ground_y_gfx = ease_out(state.ground_y_gfx, calc_height, 0.02f);

    if (player->gamemode == GAMEMODE_CUBE && !state.dual) {
        float distance = state.camera_y_lerp + (SCREEN_HEIGHT_AREA / 2) - player->y;
        float distance_abs = fabsf(distance);

        int mult = (distance >= 0 ? 1 : -1);

        float difference = player->y - state.old_player.y;

        if (distance_abs > 60.f && (difference * -mult > 0 || player->on_ground)) {
            float lerp_ratio = 0.1f;
            if (player->on_ground) {
                // Slowly make player in bounds (60 units from player center)
                state.camera_y_lerp = player->y + 60.f * mult - (SCREEN_HEIGHT_AREA / 2);
                lerp_ratio = 0.2f;
            } else {
                // Move camera
                state.camera_y_lerp += difference;
            }
            // Lerp so the camera doesn't go all the way when not moving
            state.intermediate_camera_y = ease_out(state.intermediate_camera_y, state.camera_y_lerp, lerp_ratio);
        } else {
            state.camera_y_lerp = state.intermediate_camera_y;
        }

        if (state.camera_y_lerp < -180.f) state.camera_y_lerp = -90.f;
        if (state.camera_y_lerp > MAX_LEVEL_HEIGHT) state.camera_y_lerp = MAX_LEVEL_HEIGHT;

        state.camera_y = ease_out(state.camera_y, state.intermediate_camera_y, 0.07f);
    } else {
        state.camera_y = ease_out(state.camera_y, state.camera_intended_y, 0.02f);
        state.camera_y_lerp = state.camera_y;
        state.intermediate_camera_y = state.camera_y;
    }
}

void spawn_glitter_particles() {
    if ((frame_counter & 0b1111) == 0) {
        particle_templates[GLITTER_EFFECT].angle = random_float(0, 360);
        spawn_particle(GLITTER_EFFECT, state.camera_x + SCREEN_WIDTH_AREA, state.camera_y + (SCREEN_HEIGHT_AREA / 2), NULL);
    }
}

void run_player(Player *player) {
    float scale = (player->mini) ? 0.6f : 1.f;

    if (player->gamemode != GAMEMODE_WAVE) {
        player->height = 30 * scale;
        player->width = 30 * scale;
        
        player->internal_hitbox.width = 9;
        player->internal_hitbox.height = 9;
    } else {
        player->height = 10 * scale;
        player->width = 10 * scale;

        player->internal_hitbox.width = 3;
        player->internal_hitbox.height = 3;
    }

    trail.stroke = 10.f * scale;
    
    if (!player->left_ground) {
        // Ground
        if (getGroundBottom(player) <= state.ground_y) {
            if (player->upside_down) {
                player->on_ceiling = TRUE;
                player->inverse_rotation = FALSE;
            } else {
                player->on_ground = TRUE;          
                player->inverse_rotation = FALSE;
            }
            player->time_since_ground = 0; 
        } 

        // Ceiling
        if (getGroundTop(player) >= state.ceiling_y) {
            if (player->upside_down) {
                player->on_ground = TRUE;
                player->inverse_rotation = FALSE;
            } else {
                player->on_ceiling = TRUE;     
                player->inverse_rotation = FALSE;     
            } 
            player->time_since_ground = 0; 
        } 
    }

    switch (player->gamemode) {
        case GAMEMODE_CUBE:
            cube_gamemode(player);

            if (p1_trail && (frame_counter & 0b1111) == 0) {
                particle_templates[P1_TRAIL].start_scale = 0.73333f * scale;
                particle_templates[P1_TRAIL].end_scale = 0.73333f * scale;
                spawn_particle(P1_TRAIL, player->x, player->y, NULL);
            }
            break;
        case GAMEMODE_SHIP:
            MotionTrail_ResumeStroke(&trail);
            spawn_glitter_particles();
            ship_gamemode(player);

            if (p1_trail && (frame_counter & 0b1111) == 0) {
                particle_templates[P1_TRAIL].start_scale = 0.73333f * scale / 1.8;
                particle_templates[P1_TRAIL].end_scale = 0.73333f * scale / 1.8;
                spawn_particle(P1_TRAIL, player->x, player->y, NULL);
            }
            break;
        case GAMEMODE_BALL:
            ball_gamemode(player);
            if (p1_trail && (frame_counter & 0b1111) == 0) {
                particle_templates[P1_TRAIL].start_scale = 0.73333f * scale;
                particle_templates[P1_TRAIL].end_scale = 0.73333f * scale;
                spawn_particle(P1_TRAIL, player->x, player->y, NULL);
            }
            break;
        case GAMEMODE_UFO:
            MotionTrail_ResumeStroke(&trail);
            spawn_glitter_particles();
            ufo_gamemode(player);

            if (p1_trail && (frame_counter & 0b1111) == 0) {
                particle_templates[P1_TRAIL].start_scale = 0.73333f * scale / 1.8;
                particle_templates[P1_TRAIL].end_scale = 0.73333f * scale / 1.8;
                spawn_particle(P1_TRAIL, player->x, player->y, NULL);
            }
            break;
        case GAMEMODE_WAVE:
            MotionTrail_ResumeStroke(&trail);
            spawn_glitter_particles();
            wave_gamemode(player);

            if (p1_trail && (frame_counter & 0b1111) == 0) {
                particle_templates[P1_TRAIL].start_scale = 0.73333f * scale / 1.8;
                particle_templates[P1_TRAIL].end_scale = 0.73333f * scale / 1.8;
                spawn_particle(P1_TRAIL, player->x, player->y, NULL);
            }
            break;
    }
    
    player->time_since_ground += STEPS_DT;
    
    if (player->gamemode == GAMEMODE_UFO) {
        if (player->slope_data.slope) {
            player->lerp_rotation = iSlerp(player->lerp_rotation, player->rotation, 0.05f, STEPS_DT);
        } else {
            player->lerp_rotation = iSlerp(player->lerp_rotation, player->rotation, 0.1f, STEPS_DT);
        }
    } else {
        player->lerp_rotation = iSlerp(player->lerp_rotation, player->rotation, 0.2f, STEPS_DT);
    }
    
    player->vel_x = player_speeds[state.speed];
    player->vel_y += player->gravity * STEPS_DT;
    player->y += player_get_vel(player, player->vel_y) * STEPS_DT;
    
    player->x += player->vel_x * STEPS_DT;

    player->rotation = normalize_angle(player->rotation);

    player->left_ground = FALSE;

    if (player->ceiling_inv_time > 0) {
        player->ceiling_inv_time -= STEPS_DT;
    } else {
        player->ceiling_inv_time = 0;
    }

    if (player->gamemode != GAMEMODE_WAVE) {
        if (wave_trail.opacity > 0) wave_trail.opacity -= 0.02f;
        else {
            wave_trail.opacity = 0;
            wave_trail.nuPoints = 0;
        }
    }
    
    bool slopeCheck = player->slope_data.slope && (grav_slope_orient(player->slope_data.slope, player) == 1 || grav_slope_orient(player->slope_data.slope, player) == 2);

    if (getGroundBottom(player) < state.ground_y) {
        if (player->ceiling_inv_time <= 0 && player->gamemode == GAMEMODE_CUBE && player->upside_down) {
            state.dead = TRUE;
            return;
        }

        if (slopeCheck) {
            clear_slope_data(player);
        }
        
        if (player->gamemode != GAMEMODE_WAVE) player->vel_y = 0;
        player->y = state.ground_y + (player->height / 2) + ((player->gamemode == GAMEMODE_WAVE) ? (player->mini ? 3 : 5) : 0);;
    }

    // Ceiling
    if (getGroundTop(player) > state.ceiling_y) {
        if (player->ceiling_inv_time <= 0 && player->gamemode == GAMEMODE_CUBE && !player->upside_down) {
            state.dead = TRUE;
            return;
        }

        if (slopeCheck) {
            clear_slope_data(player);
        }
        
        if (player->gamemode != GAMEMODE_WAVE) player->vel_y = 0;
        player->y = state.ceiling_y - (player->height / 2) - ((player->gamemode == GAMEMODE_WAVE) ? (player->mini ? 3 : 5) : 0);;
    } 
    
    if (player->slope_data.slope) {
        slope_calc(player->slope_data.slope, player);
    }
    
    // Ground
    if (player->gamemode == GAMEMODE_SHIP || player->gamemode == GAMEMODE_WAVE) update_ship_rotation(player);

    // End level
    if (player->x > level_info.wall_x + 30) {
        level_info.completing = TRUE;
    }
}

void handle_mirror_transition() {
    state.mirror_factor = approachf(state.mirror_factor, state.intended_mirror_factor, 0.01f, 0.002f);
    state.mirror_speed_factor = approachf(state.mirror_speed_factor, state.intended_mirror_speed_factor, 0.02f, 0.004f);
    if (state.mirror_factor >= 0.5f) {
        state.mirror_mult = -1;
    } else {
        state.mirror_mult = 1;
    }
}

void handle_player(Player *player) {
    if (state.current_player == 0) {
        set_particle_color(CUBE_DRAG, p1.r, p1.g, p1.b); 
        set_particle_color(P1_TRAIL, p1.r, p1.g, p1.b);
        set_particle_color(UFO_JUMP, p1.r, p1.g, p1.b);
        set_particle_color(UFO_TRAIL, p2.r, p2.g, p2.b);
    } else {
        set_particle_color(CUBE_DRAG, p2.r, p2.g, p2.b);  
        set_particle_color(P1_TRAIL, p2.r, p2.g, p2.b);
        set_particle_color(UFO_JUMP, p2.r, p2.g, p2.b);
        set_particle_color(UFO_TRAIL, p1.r, p1.g, p1.b);
    }
    
    if (state.input.holdA || state.input.hold2orY) {
        if (player->buffering_state == BUFFER_NONE) {
            player->buffering_state = BUFFER_READY;
        }
    } else {
        player->buffering_state = BUFFER_NONE;
    }
    
    player->on_ground = FALSE;
    player->on_ceiling = FALSE;

    player->gravObj = NULL;
    
    player->timeElapsed += STEPS_DT;

    u32 t0 = gettime();
    collide_with_objects(player);
    u32 t1 = gettime();
    collision_time = ticks_to_microsecs(t1 - t0) / 1000.f * 4.f;
    
    t0 = gettime();
    run_player(player);
    t1 = gettime();
    player_time = ticks_to_microsecs(t1 - t0) / 1000.f * 4.f;
    
    do_ball_reflection();

    if (state.hitbox_display == 2) add_new_hitbox(player);

    if (state.noclip) state.dead = FALSE;
}

void full_init_variables() {
    state.ground_x = 0;
    state.background_x = 0;

    set_particle_color(GLITTER_EFFECT, p1.r, p1.g, p1.b);
    init_variables();
}

void init_variables() {
    MotionTrail_Init(&trail_p1, 0.3f, 3, 10.0f, FALSE, p2, trail_tex);
    MotionTrail_Init(&trail_p2, 0.3f, 3, 10.0f, FALSE, p1, trail_tex);
    MotionTrail_Init(&wave_trail_p1, 3.f, 3, 10.0f, TRUE, p2, wave_trail_tex);
    MotionTrail_Init(&wave_trail_p2, 3.f, 3, 10.0f, TRUE, p1, wave_trail_tex);
    MotionTrail_StopStroke(&trail_p1);
    MotionTrail_StopStroke(&trail_p2);
    float factor_x;
    if (screenWidth <= 640)
        factor_x = 1.0f;
    else
        factor_x = 1.0f + 0.00535714f * (screenWidth - 640);

    state.camera_x = -120 * factor_x;
    state.camera_x_lerp = -120 * factor_x;
    state.camera_y = -90;
    state.camera_y_lerp = -90;
    state.intermediate_camera_y = -90;

    state.ground_y_gfx = 0;
    state.mirror_factor = 0;
    state.mirror_speed_factor = 1.f;
    state.intended_mirror_factor = 0;
    state.intended_mirror_speed_factor = 1.f;

    current_fading_effect = FADE_NONE;
    p1_trail = FALSE;
    death_timer = 0.f;

    level_info.completing = FALSE;
    
    memset(&state.player, 0, sizeof(Player));
    memset(&state.hitbox_trail_players, 0, sizeof(state.hitbox_trail_players));
    state.last_hitbox_trail = 0;

    state.dual = FALSE;
    state.dead = FALSE;

    Player *player = &state.player;
    player->width = 30;
    player->height = 30;
    state.speed = level_info.initial_speed;
    player->x = 0;
    player->y = player->height / 2;
    player->vel_x = player_speeds[state.speed];  
    player->vel_y = 0;
    state.ground_y = 0;
    state.ceiling_y = 999999;
    player->gamemode = level_info.initial_gamemode;
    player->on_ground = TRUE;
    player->on_ceiling = FALSE;
    player->inverse_rotation = FALSE;
    player->mini = level_info.initial_mini;
    player->upside_down = level_info.initial_upsidedown;
    player->timeElapsed = 0.f;

    player->internal_hitbox.height = 9;
    player->internal_hitbox.width = 9;

    switch (level_info.initial_gamemode) {
        case GAMEMODE_SHIP:
        case GAMEMODE_UFO:
        case GAMEMODE_WAVE:
            state.ceiling_y = state.ground_y + 300;
            set_intended_ceiling();
            break;
        case GAMEMODE_BALL:
            state.ceiling_y = state.ground_y + 240;
            set_intended_ceiling();
    }

    if (level_info.initial_dual) {
        state.dual = TRUE;
        state.dual_portal_y = 0.f;
        setup_dual();
    }
}

void handle_death() {
    // Spawn death particles depending on player
    if (state.current_player == 0) {
        set_particle_color(DEATH_CIRCLE, p2.r, p2.g, p2.b);
        set_particle_color(DEATH_PARTICLES, p1.r, p1.g, p1.b);

        spawn_particle(DEATH_CIRCLE, state.player.x, state.player.y, NULL);
        for (s32 i = 0; i < 20; i++) {
            spawn_particle(DEATH_PARTICLES, state.player.x, state.player.y, NULL);
        }
    } else {
        set_particle_color(DEATH_CIRCLE, p1.r, p1.g, p1.b);
        set_particle_color(DEATH_PARTICLES, p2.r, p2.g, p2.b);

        spawn_particle(DEATH_CIRCLE, state.player2.x, state.player2.y, NULL);
        for (s32 i = 0; i < 20; i++) {
            spawn_particle(DEATH_PARTICLES, state.player2.x, state.player2.y, NULL);
        }
    }

    MP3Player_Volume(0);
    PlayOgg(explode_11_ogg, explode_11_ogg_size, 0, OGG_ONE_TIME);
}

void handle_completion() {
    for (s32 i = 0; i < 240; i++) {
        draw_game();
    }
}

void load_icons() {
    icon_l1 = GRRLIB_LoadTexturePNG(player_01_001_png);
    icon_l2 = GRRLIB_LoadTexturePNG(player_01_2_001_png);
    ship_l1 = GRRLIB_LoadTexturePNG(ship_01_001_png);
    ship_l2 = GRRLIB_LoadTexturePNG(ship_01_2_001_png);
    ball_l1 = GRRLIB_LoadTexturePNG(player_ball_01_001_png);
    ball_l2 = GRRLIB_LoadTexturePNG(player_ball_01_2_001_png);
    ufo_l1 = GRRLIB_LoadTexturePNG(bird_01_001_png);
    ufo_l2 = GRRLIB_LoadTexturePNG(bird_01_2_001_png);
    ufo_dome = GRRLIB_LoadTexturePNG(bird_01_3_001_png);
    wave_l1 = GRRLIB_LoadTexturePNG(dart_01_001_png);
    wave_l2 = GRRLIB_LoadTexturePNG(dart_01_2_001_png);

    trail_tex = GRRLIB_LoadTexturePNG(trail_png);
    wave_trail_tex = GRRLIB_LoadTexturePNG(wave_trail_png);


    p1.r = 0;
    p1.g = 255;
    p1.b = 0;

    p2.r = 0;
    p2.g = 255;
    p2.b = 255;
}

void unload_icons() {
    GRRLIB_FreeTexture(icon_l1);
    GRRLIB_FreeTexture(icon_l2);
    GRRLIB_FreeTexture(ship_l1);
    GRRLIB_FreeTexture(ship_l2);
    GRRLIB_FreeTexture(ball_l1);
    GRRLIB_FreeTexture(ball_l2);
    GRRLIB_FreeTexture(ufo_l1);
    GRRLIB_FreeTexture(ufo_l2);
    GRRLIB_FreeTexture(ufo_dome);
    GRRLIB_FreeTexture(wave_l1);
    GRRLIB_FreeTexture(wave_l2);
    GRRLIB_FreeTexture(trail_tex);
    GRRLIB_FreeTexture(wave_trail_tex);
}

void draw_ship(Player *player, float calc_x, float calc_y) {
    GRRLIB_SetHandle(icon_l1, icon_l1->w / 2, icon_l1->h / 2);
    GRRLIB_SetHandle(icon_l2, icon_l2->w / 2, icon_l2->h / 2);
    GRRLIB_SetHandle(ship_l1, ship_l1->w / 2, ship_l1->h / 2);
    GRRLIB_SetHandle(ship_l2, ship_l2->w / 2, ship_l2->h / 2);
    
    float scale = ((player->mini) ? 0.6f : 1.f) * screen_factor_y;

    int mult = (player->upside_down ? -1 : 1);

    float x;
    float y;

    float calculated_scale = 0.733f * state.mirror_mult * scale;
    float calculated_rotation = player->rotation * state.mirror_mult;

    #define CUBE_DIVISOR 1.8

    rotate_point_around_center_gfx(
        get_mirror_x(calc_x, state.mirror_factor), calc_y,
        7, ((player->upside_down) ? 5 : -9) * scale,
        38, 30,
        60, 60,
        calculated_rotation,
        &x, &y
    );
    set_texture(icon_l1);
    // Top (icon)
    custom_drawImg(
        x + 6 - (30), y + 6 - (30),
        icon_l1,
        calculated_rotation,
        calculated_scale / CUBE_DIVISOR, (0.733f * scale) / CUBE_DIVISOR,
        RGBA(p1.r, p1.g, p1.b, 255)
    );

    set_texture(icon_l2);
    custom_drawImg(
        x + 6 - (30), y + 6 - (30),
        icon_l2,
        calculated_rotation,
        calculated_scale / CUBE_DIVISOR, (0.733f * scale) / CUBE_DIVISOR,
        RGBA(p2.r, p2.g, p2.b, 255)
    );

    float y_rot = (player->upside_down) ? -4 : 12;
    if (player->mini) {
        y_rot = (player->upside_down) ? 0 : 9.6;
    }
    rotate_point_around_center_gfx(
        get_mirror_x(calc_x, state.mirror_factor), calc_y,
        0, y_rot * screen_factor_y,
        38, 30,
        76, 48,
        calculated_rotation,
        &x, &y
    );

    set_texture(ship_l1);
    // Bottom (ship)
    custom_drawImg(
        x + 6 - (30), y + 6 - (30),
        ship_l1,
        calculated_rotation,
        calculated_scale, (0.733f * scale) * mult,
        RGBA(p1.r, p1.g, p1.b, 255)
    );

    set_texture(ship_l2);
    custom_drawImg(
        x + 6 - (30), y + 6 - (30),
        ship_l2,
        calculated_rotation,
        calculated_scale, (0.733f * scale) * mult,
        RGBA(p2.r, p2.g, p2.b, 255)
    );
}

void draw_ufo(Player *player, float calc_x, float calc_y) {
    GRRLIB_SetHandle(icon_l1,  icon_l1->w / 2,  icon_l1->h / 2);
    GRRLIB_SetHandle(icon_l2,  icon_l2->w / 2,  icon_l2->h / 2);
    GRRLIB_SetHandle(ufo_l1,   ufo_l1->w / 2,   ufo_l1->h / 2); 
    GRRLIB_SetHandle(ufo_l2,   ufo_l2->w / 2,   ufo_l2->h / 2); 
    GRRLIB_SetHandle(ufo_dome, ufo_dome->w / 2, ufo_dome->h / 2); 
    
    float scale = ((player->mini) ? 0.6f : 1.f) * screen_factor_y;

    int mult = (player->upside_down ? -1 : 1);

    float x;
    float y;

    float calculated_scale = 0.733f * state.mirror_mult * scale;
    float calculated_rotation = player->lerp_rotation * state.mirror_mult;

    #define CUBE_DIVISOR 1.8
    float y_rot = (player->upside_down) ? 20 : -4;
    if (player->mini) {
        y_rot = (player->upside_down) ? 16 : 2;
    }
    rotate_point_around_center_gfx(
        get_mirror_x(calc_x, state.mirror_factor), calc_y,
        6, y_rot * screen_factor_y,
        ufo_l1->w / 2, ufo_l1->h / 2,
        ufo_dome->w, ufo_dome->h,
        calculated_rotation,
        &x, &y
    );

    // Dome
    set_texture(ufo_dome);
    custom_drawImg(
        x + 6 - (30), y + 6 - (30),
        ufo_dome,
        calculated_rotation,
        calculated_scale, (0.733f * scale) * mult,
        0xffffffff
    );

    // Top (icon)
    rotate_point_around_center_gfx(
        get_mirror_x(calc_x, state.mirror_factor), calc_y,
        8, ((player->upside_down) ? 5 : -9) * scale,
        ufo_l1->w / 2, ufo_l1->h / 2,
        icon_l1->w, icon_l1->h,
        calculated_rotation,
        &x, &y
    );

    set_texture(icon_l1);

    custom_drawImg(
        x + 6 - (30), y + 6 - (30),
        icon_l1,
        calculated_rotation,
        calculated_scale / CUBE_DIVISOR, (0.733f * scale) / CUBE_DIVISOR,
        RGBA(p1.r, p1.g, p1.b, 255)
    );

    set_texture(icon_l2);
    custom_drawImg(
        x + 6 - (30), y + 6 - (30),
        icon_l2,
        calculated_rotation,
        calculated_scale / CUBE_DIVISOR, (0.733f * scale) / CUBE_DIVISOR,
        RGBA(p2.r, p2.g, p2.b, 255)
    );

    // Bottom (ufo)
    y_rot = (player->upside_down) ? 2 : 23;
    if (player->mini) {
        y_rot = (player->upside_down) ? 6 : 19;
    }
    rotate_point_around_center_gfx(
        get_mirror_x(calc_x, state.mirror_factor), calc_y,
        0, y_rot * screen_factor_y,
        ufo_l1->w / 2, ufo_l1->h / 2,
        ufo_l1->w, ufo_l1->h,
        calculated_rotation,
        &x, &y
    );

    set_texture(ufo_l1);
    custom_drawImg(
        x + 6 - (30), y + 6 - (30),
        ufo_l1,
        calculated_rotation,
        calculated_scale, (0.733f * scale) * mult,
        RGBA(p1.r, p1.g, p1.b, 255)
    );

    set_texture(ufo_l2);
    custom_drawImg(
        x + 6 - (30), y + 6 - (30),
        ufo_l2,
        calculated_rotation,
        calculated_scale, (0.733f * scale) * mult,
        RGBA(p2.r, p2.g, p2.b, 255)
    );
}


void draw_player(Player *player) {

    float calc_x = ((player->x - state.camera_x) * SCALE) - widthAdjust;
    float calc_y = screenHeight - ((player->y - state.camera_y) * SCALE);
    
    GRRLIB_SetBlend(GRRLIB_BLEND_ADD);

    MotionTrail_Update(&trail, dt);
    MotionTrail_UpdateWaveTrail(&wave_trail, dt);
    
    GX_LoadPosMtxImm(GXmodelView2D, GX_PNMTX0);

    MotionTrail_Draw(&trail);
    MotionTrail_DrawWaveTrail(&wave_trail);

    GX_SetTevOp  (GX_TEVSTAGE0, GX_MODULATE);
    GX_SetVtxDesc(GX_VA_TEX0,   GX_DIRECT);

    GRRLIB_SetBlend(GRRLIB_BLEND_ALPHA);

    draw_particles(P1_TRAIL);

    float scale = (player->mini) ? 0.6f : 1.f;

    switch (player->gamemode) {
        case GAMEMODE_CUBE:
            GRRLIB_SetHandle(icon_l1, 30, 30);
            GRRLIB_SetHandle(icon_l2, 30, 30);
        
            set_texture(icon_l1);
            custom_drawImg(
                get_mirror_x(calc_x, state.mirror_factor) + 6 - (30), calc_y + 6 - (30),
                icon_l1,
                player->lerp_rotation * state.mirror_mult,
                BASE_SCALE * state.mirror_mult * scale,
                BASE_SCALE * scale,
                RGBA(p1.r, p1.g, p1.b, 255)
            );

            set_texture(icon_l2);
            custom_drawImg(
                get_mirror_x(calc_x, state.mirror_factor) + 6 - (30), calc_y + 6 - (30),
                icon_l2,
                player->lerp_rotation * state.mirror_mult,
                BASE_SCALE * state.mirror_mult * scale,
                BASE_SCALE * scale,
                RGBA(p2.r, p2.g, p2.b, 255)
            );
            break;
        case GAMEMODE_SHIP:
            draw_ship(player, calc_x - 8 * state.mirror_mult, calc_y);
            break;
        case GAMEMODE_BALL:
            GRRLIB_SetHandle(ball_l1, 36, 36);
            GRRLIB_SetHandle(ball_l2, 36, 36);
            set_texture(ball_l1);
            custom_drawImg(
                get_mirror_x(calc_x, state.mirror_factor) + 6 - (36), calc_y + 6 - (36),
                ball_l1,
                player->lerp_rotation * state.mirror_mult,
                BASE_SCALE * state.mirror_mult * scale,
                BASE_SCALE * scale,
                RGBA(p1.r, p1.g, p1.b, 255)
            );

            set_texture(ball_l2);
            custom_drawImg(
                get_mirror_x(calc_x, state.mirror_factor) + 6 - (36), calc_y + 6 - (36),
                ball_l2,
                player->lerp_rotation * state.mirror_mult,
                BASE_SCALE * state.mirror_mult * scale,
                BASE_SCALE * scale,
                RGBA(p2.r, p2.g, p2.b, 255)
            );
            break;
        case GAMEMODE_UFO:
            draw_ufo(player, calc_x - 8 * state.mirror_mult, calc_y);
            break;
        case GAMEMODE_WAVE:
            GRRLIB_SetHandle(wave_l1, wave_l1->w / 2, wave_l1->h / 2);
            GRRLIB_SetHandle(wave_l2, wave_l2->w / 2, wave_l2->h / 2);
            set_texture(wave_l1);
            custom_drawImg(
                get_mirror_x(calc_x, state.mirror_factor) + 6 - (wave_l1->w / 2), calc_y + 6 - (wave_l1->h / 2),
                wave_l1,
                player->lerp_rotation * state.mirror_mult,
                BASE_SCALE * state.mirror_mult * scale,
                BASE_SCALE * scale,
                RGBA(p1.r, p1.g, p1.b, 255)
            );

            set_texture(wave_l2);
            custom_drawImg(
                get_mirror_x(calc_x, state.mirror_factor) + 6 - (wave_l2->w / 2), calc_y + 6 - (wave_l2->h / 2),
                wave_l2,
                player->lerp_rotation * state.mirror_mult,
                BASE_SCALE * state.mirror_mult * scale,
                BASE_SCALE * scale,
                RGBA(p2.r, p2.g, p2.b, 255)
            );
            break;
    }
    set_texture(prev_tex);
}

GRRLIB_texImg *get_p1_trail_tex() {
    switch (state.player.gamemode) {
        case GAMEMODE_CUBE:
            return icon_l1;
        
        case GAMEMODE_BALL:
            return ball_l1;

        case GAMEMODE_WAVE:
            return wave_l1;
        
    }
    return icon_l1;
}

const float ejections[SPEED_COUNT] = {
    316.4617f,
    392.5909f,
    488.149356f,
    589.68013f
};

const float falls[SPEED_COUNT] = {
    226.044054,
    280.422108,
    348.678108,
    421.200108
};

void clear_slope_data(Player *player) {
    player->slope_data.slope = NULL;
    player->slope_data.elapsed = 0.f;
    player->slope_data.snapDown = FALSE;
}

int grav_slope_orient(GameObject *obj, Player *player) {
    int orient = obj->orientation;

    if (player->upside_down) {
        if (orient == 3)
			orient = 0;
		else if (orient == 2)
			orient = 1;
		else if (orient == 0)
			orient = 3;
		else if (orient == 1)
			orient = 2;
    }
    return orient;
}

bool is_spike_slope(GameObject *obj) {
    switch (obj->id) {
        case GROUND_SPIKE_SLOPE_45:
        case GROUND_SPIKE_SLOPE_22_66:
        case WAVY_GROUND_SPIKE_SLOPE_45:
        case WAVY_GROUND_SPIKE_SLOPE_22_66:
            return TRUE;
    }
    return FALSE;
}

float slope_angle(GameObject *obj, Player *player) {
    float angle = atanf((float) obj->height / obj->width);
    int orient = grav_slope_orient(obj, player);
    if (orient == 1 || orient == 2) {
        angle = -angle;
    }

    return angle;
}

float get_slope_angle(GameObject *obj) {
    float angle = atanf((float) obj->height / obj->width);
    return angle;
}

float slope_snap_angle(GameObject *obj, Player *player) {
    float angle = slope_angle(obj, player);
    int orient = obj->orientation;

    if (orient == 0) angle = -fabsf(angle);
    if (orient == 1) angle = fabsf(angle);

    return angle;
}

float expected_slope_y(GameObject *obj, Player *player) {
    int flipping = grav_slope_orient(obj, player) >= 2;
    int mult = (player->upside_down ^ flipping) ? -1 : 1;
    
    float angle = slope_angle(obj, player);
    float ydist = mult * player->height * sqrtf(powf(tanf(angle), 2) + 1) / 2;
    float pos_relative = ((float) obj->height / obj->width) * (player->x - obj_getLeft(obj));

    if ((angle > 0) ^ player->upside_down ^ flipping) {
        return obj_getBottom(obj) + MIN(pos_relative + ydist, obj->height + player->height / 2);
    } else {
        return obj_getTop(obj) - MAX(pos_relative - ydist, -player->height / 2);
    }
}

const float slope_multiplier[SPEED_COUNT] = {
    0.7f,
    0.9f,
    1.1f,
    1.3f,
};

void slope_calc(GameObject *obj, Player *player) {
    int orientation = grav_slope_orient(obj, player);
    if (orientation == 0) {
        // Handle leaving slope
        if (!slope_touching(obj, player)) {
            clear_slope_data(player);
            return;
        }

        if (player->gamemode == GAMEMODE_WAVE) {
            state.dead = TRUE;
            clear_slope_data(player);
            return;
        }

        // On slope
        if (gravBottom(player) != obj_gravTop(player, obj)) {
            if (is_spike_slope(obj)) {
                state.dead = TRUE;
            }

            if (player->upside_down) {
                player->y = MAX(obj_getBottom(obj) - player->height / 2, expected_slope_y(obj, player));
            } else {
                player->y = MIN(obj_getTop(obj) + player->height / 2, expected_slope_y(obj, player));
            }

            player->time_since_ground = 0;
            snap_player_to_slope(obj, player);
            
            if (player->vel_y < 0) {
                player->vel_y = 0;
            }
        }

        // Sliding off slope
        if (gravBottom(player) >= obj_gravTop(player, obj)) {
            float vel = ejections[state.speed] * ((float) obj->height / obj->width) * MIN(1.1f, 0.8f / slope_angle(obj, player));
            float time = clampf(10 * (player->timeElapsed - player->slope_data.elapsed) * slope_multiplier[state.speed], 0.4f, 1.0f);
            
            if (player->gamemode == GAMEMODE_BALL) {
                vel *= 0.75f;
            }
            
            if (player->gamemode == GAMEMODE_SHIP) {
                vel *= 0.75f;
            }

            if (player->gamemode == GAMEMODE_UFO) {
                vel *= 0.7499f;
            }

            vel *= time;

            player->vel_y = vel;
            player->inverse_rotation = TRUE;
            clear_slope_data(player);
        }
    } else if (orientation == 1) {
        // Handle leaving slope
        if (player->vel_y > 0) {
            clear_slope_data(player);
            return;
        }
        
        if (player->gamemode == GAMEMODE_WAVE) {
            state.dead = TRUE;
            clear_slope_data(player);
            return;
        }

        if (gravBottom(player) != obj_gravTop(player, obj) || player->slope_data.snapDown) {
            if (is_spike_slope(obj)) {
                state.dead = TRUE;
            }

            if (player->upside_down) {
                player->y = MIN(expected_slope_y(obj, player), obj_getTop(obj) + player->height / 2);
            } else {
                player->y = MAX(expected_slope_y(obj, player), obj_getBottom(obj) - player->height / 2);
            }
            
            player->time_since_ground = 0;
            snap_player_to_slope(obj, player);
            if (player->vel_y < 0) {
                player->vel_y = 0;
            }
        }

        if (obj_gravTop(player, obj) <= grav(player, player->y) || getLeft(player) - obj_getRight(obj) > 0) {
            float vel = -falls[state.speed] * ((float) obj->height / obj->width);
            player->vel_y = vel;
            clear_slope_data(player);
        }
    } else if (orientation == 3) {
        // Handle leaving slope
        if (!slope_touching(obj, player)) {
            clear_slope_data(player);
            return;
        }
        
        bool gravSnap = (player->ceiling_inv_time > 0) || (player->gravObj && player->gravObj->hitbox_counter[state.current_player] == 1);
        
        if (player->gamemode == GAMEMODE_CUBE && !gravSnap) {
            state.dead = TRUE;
        }

        if (player->gamemode == GAMEMODE_WAVE) {
            state.dead = TRUE;
            clear_slope_data(player);
            return;
        }

        // On slope
        if (gravBottom(player) != obj_gravTop(player, obj)) {
            if (is_spike_slope(obj)) {
                state.dead = TRUE;
            }

            if (player->upside_down) {
                player->y = MIN(obj_getTop(obj) + player->height / 2, expected_slope_y(obj, player));
            } else {
                player->y = MAX(obj_getBottom(obj) - player->height / 2, expected_slope_y(obj, player));
            }

            player->time_since_ground = 0;
            snap_player_to_slope(obj, player);
            
            if (player->vel_y > 0) {
                player->vel_y = 0;
            }
        }

        // Sliding off slope
        if (gravTop(player) <= obj_gravBottom(player, obj)) {
            float vel = ejections[state.speed] * ((float) obj->height / obj->width) * MIN(1.1f, 0.8f / slope_angle(obj, player));
            float time = clampf(10 * (player->timeElapsed - player->slope_data.elapsed) * slope_multiplier[state.speed], 0.4f, 1.0f);
            
            //float orig = vel;
            if (player->gamemode == GAMEMODE_BALL) {
                vel *= 0.75f;
            }
            
            if (player->gamemode == GAMEMODE_SHIP) {
                vel *= 0.75f;
            }

            if (player->gamemode == GAMEMODE_UFO) {
                vel *= 0.7499f;
            }

            vel *= time;

            player->vel_y = -vel;
            //printf("vel %.2f orig %.2f time %.2f y %.2f obj_y %.2f\n", -vel, -orig, time, player->y, obj->y);
            player->inverse_rotation = TRUE;
            clear_slope_data(player);
        }
    } else {
        // Handle leaving slope
        if (player->vel_y < 0) {
            clear_slope_data(player);
            return;
        }
        
        bool gravSnap = (player->ceiling_inv_time > 0) || (player->gravObj && player->gravObj->hitbox_counter[state.current_player] == 1);
        
        if (player->gamemode == GAMEMODE_CUBE && !gravSnap) {
            state.dead = TRUE;
        }
        
        if (player->gamemode == GAMEMODE_WAVE) {
            state.dead = TRUE;
            clear_slope_data(player);
            return;
        }

        if (gravTop(player) != obj_gravBottom(player, obj) || player->slope_data.snapDown) {
            if (is_spike_slope(obj)) {
                state.dead = TRUE;
            }

            if (player->upside_down) {
                player->y = MAX(expected_slope_y(obj, player), obj_getBottom(obj) - player->height / 2);
            } else {
                player->y = MIN(expected_slope_y(obj, player), obj_getTop(obj) + player->height / 2);
            }
            
            player->time_since_ground = 0;
            snap_player_to_slope(obj, player);
            if (player->vel_y > 0) {
                player->vel_y = 0;
            }
        }

        if (obj_gravTop(player, obj) <= grav(player, player->y) || getLeft(player) - obj_getRight(obj) > 0) {
            float vel = falls[state.speed] * ((float) obj->height / obj->width);
            player->vel_y = vel;
            clear_slope_data(player);
        }
    }
}

bool player_circle_touches_slope(GameObject *obj, Player *player) {
    float x1, y1, x2, y2;
    int orientation = obj->orientation;

    float hw = obj->width / 2.f, hh = obj->height / 2.f;

    switch (orientation) {
        case 0:
        case 2:
            x1 = obj->x - hw;
            y1 = obj->y - hh;
            x2 = obj->x + hw;
            y2 = obj->y + hh;
            break;
        case 1:
        case 3:
            x1 = obj->x + hw;
            y1 = obj->y - hh;
            x2 = obj->x - hw;
            y2 = obj->y + hh;
            break;
        default:
            x1 = y1 = x2 = y2 = 0;
            break;
    }
    bool collided_hipo = circle_rect_collision(player->x, player->y, player->width / 2, x1, y1, x2, y2);

    // Collide with vertical
    switch (orientation) {
        case 0:
        case 3:
            x1 = obj->x + hw;
            y1 = obj->y - hh;
            x2 = obj->x + hw;
            y2 = obj->y + hh;
            break;
        case 1:
        case 2:
            x1 = obj->x - hw;
            y1 = obj->y - hh;
            x2 = obj->x - hw;
            y2 = obj->y + hh;
            break;
        default:
            x1 = y1 = x2 = y2 = 0;
            break;
    }
    
    bool collided_vertical = circle_rect_collision(player->x, player->y, player->width / 2, x1, y1, x2, y2);

    // Collide with horizontal
    switch (orientation) {
        case 0:
        case 1:
            x1 = obj->x + hw;
            y1 = obj->y - hh;
            x2 = obj->x - hw;
            y2 = obj->y - hh;
            break;
        case 2:
        case 3:
            x1 = obj->x + hw;
            y1 = obj->y + hh;
            x2 = obj->x - hw;
            y2 = obj->y + hh;
            break;
        default:
            x1 = y1 = x2 = y2 = 0;
            break;
    }
    
    bool collided_horizontal = circle_rect_collision(player->x, player->y, player->width / 2, x1, y1, x2, y2);

    return collided_vertical | collided_hipo | collided_horizontal;
}

void slope_collide(GameObject *obj, Player *player) {
    int clip = (player->gamemode == GAMEMODE_SHIP || player->gamemode == GAMEMODE_UFO) ? 7 : 10;
    int orient = grav_slope_orient(obj, player);  
    int mult = orient >= 2 ? -1 : 1;

    InternalHitbox internal = player->internal_hitbox;

    
    bool gravSnap = (player->ceiling_inv_time > 0) || (player->gravObj && player->gravObj->hitbox_counter[state.current_player] == 1);

    if (!gravSnap && player->gamemode == GAMEMODE_CUBE && grav_slope_orient(obj, player) >= 2 && !player_circle_touches_slope(obj, player)) return;

    // Check if player inside slope
    if (orient == 0 || orient == 3) {
        bool internalCollidingSlope = intersect(
            player->x, player->y, internal.width, internal.height, 0, 
            obj_getRight(obj), obj->y, 1, obj->height, 0
        );

        if (internalCollidingSlope) state.dead = TRUE;
    }

    // Normal slope - resting on bottom
    if (
        !state.old_player.slope_data.slope &&
        orient < 2 && 
        gravTop(player) - obj_gravBottom(player, obj) <= clip + 5 * !player->mini // Remove extra if mini
    ) {
        if (player->gamemode != GAMEMODE_WAVE && ((player->gamemode != GAMEMODE_CUBE && (player->vel_y >= 0)) || gravSnap)) {
            player->vel_y = 0;
            if (!gravSnap) player->on_ceiling = TRUE;
            player->time_since_ground = 0;
            player->y = grav(player, obj_gravBottom(player, obj)) - grav(player, player->height / 2);
        } else {
            bool internalCollidingSlope = intersect(
                player->x, player->y, internal.width, internal.height, 0, 
                obj->x, obj->y, obj->width, obj->height, 0
            );

            if (internalCollidingSlope) state.dead = TRUE;
        }

        return;
    }

    // Upside down slope - resting on top
    if (
        !state.old_player.slope_data.slope &&
        orient >= 2 && 
        obj_gravTop(player, obj) - gravBottom(player) <= clip + 5 * !player->mini // Remove extra if mini
    ) {
        if (player->gamemode != GAMEMODE_WAVE && player->vel_y <= 0) {
            player->vel_y = 0;
            if (!gravSnap) player->on_ground = TRUE;
            player->time_since_ground = 0;
            player->y = grav(player, obj_gravTop(player, obj)) + grav(player, player->height / 2);
        } else {
            bool internalCollidingSlope = intersect(
                player->x, player->y, internal.width, internal.height, 0, 
                obj->x, obj->y, obj->width, obj->height, 0
            );

            if (internalCollidingSlope) state.dead = TRUE;
        }
        
        return;
    }

    // Left side collision
    if (
        !state.old_player.slope_data.slope && 
        (orient == 1 || orient == 2) && 
        player->x - obj_getLeft(obj) < 0
    ) {
        // Going from the left
        if (obj_gravTop(player, obj) - gravBottom(player) > clip) {
            bool internalCollidingSlope = intersect(
                player->x, player->y, internal.width, internal.height, 0, 
                obj->x, obj->y, obj->width, obj->height, 0
            );

            if (internalCollidingSlope) state.dead = TRUE;
            return;
        }
        
        // Touching slope before center is in slope
        if (player->gamemode != GAMEMODE_WAVE && player->vel_y * mult <= 0) {
            if (orient == 1) {
                player->y = grav(player, obj_gravTop(player, obj)) + grav(player, player->height / 2);
            } else {
                player->y = grav(player, obj_gravBottom(player, obj)) - grav(player, player->height / 2);
            }
            player->on_ground = TRUE;
            player->inverse_rotation = FALSE;
            return;
        }
    }
    bool colliding = intersect(
        player->x, player->y, player->width, player->height, 0, 
        obj->x, obj->y, obj->width, obj->height, 0
    );

    GameObject *slope = player->slope_data.slope;
    if (
        (!slope || grav_slope_orient(slope, player) == grav_slope_orient(obj, player) ||
            (
                grav_slope_orient(slope, player) != grav_slope_orient(obj, player) && 
                (
                    (grav_slope_orient(slope, player) == 1 && grav_slope_orient(obj, player) == 0) ||
                    (grav_slope_orient(slope, player) == 2 && grav_slope_orient(obj, player) == 3)
                )
            ) 
        ) && slope_touching(obj, player) && colliding && obj_gravTop(player, obj) - gravBottom(player) > 0.06
    ) {
        
        if (slope && slope_angle(obj, player) < slope_angle(slope, player)) return;
        float angle = atanf((player->vel_y * STEPS_DT) / (player_speeds[state.speed] * STEPS_DT));
        
        if (orient >= 2) angle = -angle;

        bool hasSlope = state.old_player.slope_data.slope;
        if (hasSlope && slope) {
            hasSlope = state.old_player.slope_data.slope->orientation == slope->orientation;
        }
        

        #define SLOPE_EPSILON 3
        bool projectedHit = (orient == 1 || orient == 2) ? (angle * 1.1f <= slope_angle(obj, player)) : (angle <= slope_angle(obj, player));
        bool clip = slope_touching(obj, player);
        bool snapDown = (orient == 1 || orient == 2) && player->vel_y * mult > 0 && player->x - obj_getLeft(obj) > 0;

        //printf("p %d - orient %d, slope angle %.2f - hasSlope %d, vel_y %d, projectedHit %d clip %d snapDown %d (clip val %.2f)\n", state.current_player, orient, slope_angle(obj,player), hasSlope, player->vel_y * mult <= 0, projectedHit, clip, snapDown, grav(player, player->y) - grav(player, expected_slope_y(obj, player)));
        
        if (hasSlope ? player->vel_y * mult <= 0 : (projectedHit && clip) || snapDown) {
            player->on_ground = TRUE;
            player->inverse_rotation = FALSE;
            player->slope_data.slope = obj;
            snap_player_to_slope(obj, player);

            if (snapDown && !hasSlope) {
                if (orient == 1 && player->vel_y <= 0) {
                    player->y = grav(player, obj_gravTop(player, obj)) + grav(player, player->height / 2);
                    player->slope_data.snapDown = TRUE;
                    player->vel_y = 0;
                } else if (orient == 2 && player->vel_y >= 0) {
                    player->y = grav(player, obj_gravBottom(player, obj)) - grav(player, player->height / 2);
                    player->slope_data.snapDown = TRUE;
                    player->vel_y = 0;
                }
            }

            if (!player->slope_data.elapsed) {
                player->slope_data.elapsed = state.old_player.timeElapsed;
            }
        } 
    }
}

bool slope_touching(GameObject *obj, Player *player) {
    switch (grav_slope_orient(obj, player)) {
        case 0:
        case 1:
            float diff = grav(player, expected_slope_y(obj, player)) - grav(player, player->y);
            return diff >= 0 && diff <= 10;
        case 2:
        case 3:
            float diff_ud = grav(player, player->y) - grav(player, expected_slope_y(obj, player));  
            return diff_ud >= 0 && diff_ud <= 10;
        default:
            return false;
    }
}

void snap_player_to_slope(GameObject *obj, Player *player) {
    if (player->gamemode == GAMEMODE_CUBE) {
        float base = RadToDeg(slope_snap_angle(obj, player));
        float bestSnap = base;
        float minDiff = 999999.0f;

        for (int i = 0; i < 4; ++i) {
            float snapAngle = base + i * 90.0f;
            
            while (snapAngle < 0) snapAngle += 360.0f;
            while (snapAngle >= 360.0f) snapAngle -= 360.0f;

            float diff = fabsf(snapAngle - (player->rotation - 0.01f));
            if (diff > 180.0f) diff = 360.0f - diff;

            if (diff < minDiff) {
                minDiff = diff;
                bestSnap = snapAngle;
            }
        }

        //printf("best snap %.2f prev rotation %.2f\n", bestSnap, player->rotation);
        player->rotation = bestSnap;
    } else if (player->gamemode == GAMEMODE_UFO) {
        player->rotation = RadToDeg(slope_snap_angle(obj, player));
    }
}

float calc_x_on_screen(float val) {
    if (state.mirror_factor >= 0.5f) {
        return screenWidth - (((val - state.camera_x) * SCALE) - widthAdjust) + 6;
    } else {
        return ((val - state.camera_x) * SCALE) - widthAdjust + 6;
    }
}
float calc_y_on_screen(float val) {
    return screenHeight - ((val - state.camera_y) * SCALE) + 6;
}

extern void get_corners(float cx, float cy, float w, float h, float angle, Vec2D out[4]);

void draw_triangle_from_rect(Vec2D rect[4], int skip_index, uint32_t color) {
    Vec2D tri[3];
    int ti = 0;

    // Collect 3 points that are not the skipped one
    for (int i = 0; i < 4; ++i) {
        if (i == skip_index) continue;
        tri[ti++] = rect[i];
    }

    // Draw 3 lines to form a closed triangle
    for (int i = 0; i < 3; ++i) {
        Vec2D a = tri[i];
        Vec2D b = tri[(i + 1) % 3];  // Wrap around to connect last to first
        draw_thick_line(
            calc_x_on_screen(a.x), calc_y_on_screen(a.y),
            calc_x_on_screen(b.x), calc_y_on_screen(b.y),
            2, color
        );
    }
}
void draw_hitbox(GameObject *obj) {
    ObjectHitbox hitbox = objects[obj->id].hitbox;

    float x = obj->x;
    float y = obj->y;
    float w = hitbox.width;
    float h = hitbox.height;
    float angle = obj->rotation;

    unsigned int color = RGBA(0x00, 0xff, 0xff, 0xff);

    int hitbox_type = hitbox.type;
    if (hitbox_type == HITBOX_SPIKE) color = RGBA(0xff, 0x00, 0x00, 0xff);
    if (hitbox_type == HITBOX_SOLID) color = RGBA(0x00, 0x00, 0xff, 0xff);
    
    if (obj == state.player.slope_data.slope || obj == state.player2.slope_data.slope) color = RGBA(0x00, 0xff, 0x00, 0xff);

    Vec2D rect[4];
    if (objects[obj->id].is_slope) {
        w = obj->width;
        h = obj->height;
        get_corners(x, y, w, h, 0, rect);

        draw_triangle_from_rect(rect, 3 - obj->orientation,color);
    } else if (objects[obj->id].is_saw) {
        if (hitbox.radius == 0) return;

        float calc_radius = hitbox.radius * SCALE;

        custom_circle(calc_x_on_screen(x), calc_y_on_screen(y), calc_radius, color, FALSE);
        custom_circle(calc_x_on_screen(x), calc_y_on_screen(y), calc_radius - 1, color, FALSE);
    } else {
        if (w == 0 || h == 0) return;

        get_corners(x, y, w, h, angle, rect);
        draw_thick_line(calc_x_on_screen(rect[0].x), calc_y_on_screen(rect[0].y), calc_x_on_screen(rect[1].x), calc_y_on_screen(rect[1].y), 2, color);
        draw_thick_line(calc_x_on_screen(rect[1].x), calc_y_on_screen(rect[1].y), calc_x_on_screen(rect[2].x), calc_y_on_screen(rect[2].y), 2, color);
        draw_thick_line(calc_x_on_screen(rect[2].x), calc_y_on_screen(rect[2].y), calc_x_on_screen(rect[3].x), calc_y_on_screen(rect[3].y), 2, color);
        draw_thick_line(calc_x_on_screen(rect[3].x), calc_y_on_screen(rect[3].y), calc_x_on_screen(rect[0].x), calc_y_on_screen(rect[0].y), 2, color);
    }
}

void add_new_hitbox(Player *player) {
    for (int i = HITBOX_TRAIL_SIZE - 2; i > 0; i--) {
        state.hitbox_trail_players[state.current_player][i] = state.hitbox_trail_players[state.current_player][i - 1];
    }
    PlayerHitboxTrail hitbox;
    hitbox.x = player->x;
    hitbox.y = player->y;
    hitbox.width = player->width;
    hitbox.height = player->height;
    hitbox.rotation = player->rotation;
    hitbox.internal_hitbox = player->internal_hitbox;

    state.hitbox_trail_players[state.current_player][0] = hitbox;

    state.last_hitbox_trail++;

    if (state.last_hitbox_trail >= HITBOX_TRAIL_SIZE) state.last_hitbox_trail = HITBOX_TRAIL_SIZE - 1;
}

void draw_hitbox_trail(int player) {
    for (int i = state.last_hitbox_trail - 1; i >= 0; i--) {
        PlayerHitboxTrail hitbox = state.hitbox_trail_players[player][i];

        Player player;
        player.x = hitbox.x;
        player.y = hitbox.y;
        player.width = hitbox.width;
        player.height = hitbox.height;
        player.internal_hitbox = hitbox.internal_hitbox;
        player.rotation = hitbox.rotation;

        draw_player_hitbox(&player);
    }
}

void draw_player_hitbox(Player *player) {
    InternalHitbox internal = player->internal_hitbox;
    Vec2D rect[4];
    // Rotated hitbox
    get_corners(player->x, player->y, player->width, player->height, player->rotation, rect);

    draw_thick_line(calc_x_on_screen(rect[0].x), calc_y_on_screen(rect[0].y), calc_x_on_screen(rect[1].x), calc_y_on_screen(rect[1].y), 2, RGBA(0x7f, 0x00, 0x00, 0xff));
    draw_thick_line(calc_x_on_screen(rect[1].x), calc_y_on_screen(rect[1].y), calc_x_on_screen(rect[2].x), calc_y_on_screen(rect[2].y), 2, RGBA(0x7f, 0x00, 0x00, 0xff));
    draw_thick_line(calc_x_on_screen(rect[2].x), calc_y_on_screen(rect[2].y), calc_x_on_screen(rect[3].x), calc_y_on_screen(rect[3].y), 2, RGBA(0x7f, 0x00, 0x00, 0xff));
    draw_thick_line(calc_x_on_screen(rect[3].x), calc_y_on_screen(rect[3].y), calc_x_on_screen(rect[0].x), calc_y_on_screen(rect[0].y), 2, RGBA(0x7f, 0x00, 0x00, 0xff));

    // Internal hitbox
    get_corners(player->x, player->y, internal.width, internal.height, 0, rect);

    draw_thick_line(calc_x_on_screen(rect[0].x), calc_y_on_screen(rect[0].y), calc_x_on_screen(rect[1].x), calc_y_on_screen(rect[1].y), 2, RGBA(0x00, 0x00, 0x7f, 0xff));
    draw_thick_line(calc_x_on_screen(rect[1].x), calc_y_on_screen(rect[1].y), calc_x_on_screen(rect[2].x), calc_y_on_screen(rect[2].y), 2, RGBA(0x00, 0x00, 0x7f, 0xff));
    draw_thick_line(calc_x_on_screen(rect[2].x), calc_y_on_screen(rect[2].y), calc_x_on_screen(rect[3].x), calc_y_on_screen(rect[3].y), 2, RGBA(0x00, 0x00, 0x7f, 0xff));
    draw_thick_line(calc_x_on_screen(rect[3].x), calc_y_on_screen(rect[3].y), calc_x_on_screen(rect[0].x), calc_y_on_screen(rect[0].y), 2, RGBA(0x00, 0x00, 0x7f, 0xff));
    
    // Circle hitbox
    float calc_radius = (player->width / 2) * SCALE;
    custom_circle(calc_x_on_screen(player->x), calc_y_on_screen(player->y), calc_radius, RGBA(0xff, 0x00, 0x00, 0xff), FALSE);
    custom_circle(calc_x_on_screen(player->x), calc_y_on_screen(player->y), calc_radius - 1, RGBA(0xff, 0x00, 0x00, 0xff), FALSE);

    // Unrotated hitbox
    get_corners(player->x, player->y, player->width, player->height, 0, rect);

    draw_thick_line(calc_x_on_screen(rect[0].x), calc_y_on_screen(rect[0].y), calc_x_on_screen(rect[1].x), calc_y_on_screen(rect[1].y), 2, RGBA(0xff, 0x00, 0x00, 0xff));
    draw_thick_line(calc_x_on_screen(rect[1].x), calc_y_on_screen(rect[1].y), calc_x_on_screen(rect[2].x), calc_y_on_screen(rect[2].y), 2, RGBA(0xff, 0x00, 0x00, 0xff));
    draw_thick_line(calc_x_on_screen(rect[2].x), calc_y_on_screen(rect[2].y), calc_x_on_screen(rect[3].x), calc_y_on_screen(rect[3].y), 2, RGBA(0xff, 0x00, 0x00, 0xff));
    draw_thick_line(calc_x_on_screen(rect[3].x), calc_y_on_screen(rect[3].y), calc_x_on_screen(rect[0].x), calc_y_on_screen(rect[0].y), 2, RGBA(0xff, 0x00, 0x00, 0xff));
}