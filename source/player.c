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

#include "easing.h"

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

MotionTrail trail;
MotionTrail trail_p1;
MotionTrail trail_p2;

MotionTrail wave_trail;
MotionTrail wave_trail_p1;
MotionTrail wave_trail_p2;

Color p1;
Color p2;

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
                // Something that makes the slope not reset the speed
                bottom = bottom + sinf(slope_angle(player->slope_data.slope, player)) * player->height / 2;
                clip = 7;
                if (obj_gravTop(player, obj) - bottom < 2)
                    return;
            }
            
            // Collide with slope if object is an slope
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

            bool slope_height_check = FALSE;
            if (player->touching_slope) {
                if (grav_slope_orient(player->potentialSlope, player) == ORIENT_NORMAL_DOWN) {
                    slope_height_check = gravBottom(player) < grav(player, player->potentialSlope->y);
                } else if (grav_slope_orient(player->potentialSlope, player) == ORIENT_UD_DOWN) {
                    slope_height_check = gravTop(player) > grav(player, player->potentialSlope->y);
                }
            }

            bool slope_condition = player->touching_slope && !slope_touching(player->potentialSlope, player) && slope_height_check && (player->potentialSlope->object.orientation == ORIENT_NORMAL_DOWN || player->potentialSlope->object.orientation == ORIENT_UD_DOWN);

            // Snap the player to the potential slope when the player is touching the slope
            if (player->touching_slope && slope_touching(player->potentialSlope, player) && slope_height_check) {
                slope_collide(player->potentialSlope, player);
                break;
            }
            
            bool safeZone = player->mini && ((obj_gravTop(player, obj) - gravBottom(player) <= clip) || (gravTop(player) - obj_gravBottom(player, obj) <= clip));
            
            if ((player->gamemode == GAMEMODE_WAVE || (!gravSnap && !safeZone)) && intersect(
                player->x, player->y, internal.width, internal.height, 0, 
                obj->x, obj->y, hitbox->width, hitbox->height, obj->rotation
            )) {
                if (hitbox->type == HITBOX_BREAKABLE_BLOCK) {
                    // Spawn breakable brick particles
                    obj->toggled = TRUE;
                    for (s32 i = 0; i < 10; i++) {
                        spawn_particle(BREAKABLE_BRICK_PARTICLES, obj->x, obj->y, obj);
                    }
                } else {
                    // Not a brick, die
                    state.dead = TRUE;
                }
            // Check snap for player bottom
            } else if (obj_gravTop(player, obj) - gravBottom(player) <= clip && player->vel_y <= CLAMP(obj->object.delta_y, 3000, 0) && !slope_condition && player->gamemode != GAMEMODE_WAVE) {
                player->y = grav(player, obj_gravTop(player, obj)) + grav(player, player->height / 2);
                player->vel_y = 0;
                obj->object.touching_player = state.current_player + 1;
                player->on_ground = TRUE;
                player->inverse_rotation = FALSE;
                player->time_since_ground = 0;
            // Check snap for player top
            } else if (player->gamemode != GAMEMODE_WAVE) {
                // Ufo can break breakable blocks from above, so dont use as a ceiling
                if (player->gamemode == GAMEMODE_UFO && hitbox->type == HITBOX_BREAKABLE_BLOCK) {
                    break;
                }
                // Behave normally
                if (player->gamemode != GAMEMODE_CUBE || gravSnap) {
                    if (((gravTop(player) - obj_gravBottom(player, obj) <= clip && player->vel_y > obj->object.delta_y) || gravSnap) && !slope_condition) {
                        if (!gravSnap) player->on_ceiling = TRUE;
                        player->inverse_rotation = FALSE;
                        player->time_since_ground = 0;
                        player->ceiling_inv_time = 0;
                        obj->object.touching_player = state.current_player + 1;
                        player->y = grav(player, obj_gravBottom(player, obj)) - grav(player, player->height / 2);
                        player->vel_y = 0;
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
        obj->object.prev_touching_player = obj->object.touching_player;
        obj->object.touching_player = 0;
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

void collide_with_slope(Player *player, GameObject *obj, bool has_slope) {
    ObjectHitbox *hitbox = (ObjectHitbox *) &objects[obj->id].hitbox;
    number_of_collisions_checks++;
    if (intersect(
        player->x, player->y, player->width, player->height, 0, 
        obj->x, obj->y, hitbox->width, hitbox->height, obj->rotation
    )) {
        // The same check in handle_collision
        if (has_slope) {
            float bottom = gravBottom(player) + sinf(slope_angle(player->slope_data.slope, player)) * player->height / 2;
            if (obj_gravTop(player, obj) - bottom < 2)
                return;
        }
        slope_collide(obj, player);
        number_of_collisions++;
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
                
                // Save some types to buffer, so they can be checked in a type order
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
            float dist = fabsf(obj->y - player->y);
            if (dist < closestDist) {
                player->touching_slope = TRUE;
                player->potentialSlope = obj;
                closestDist = dist; 
            }
        }
    }

    for (int i = 0; i < block_count; i++) {
        GameObject *obj = block_buffer[i];
        collide_with_obj(player, obj);
    }

    bool has_slope = player->slope_data.slope;
    for (int i = 0; i < slope_count; i++) {
        GameObject *obj = slope_buffer[i];
        collide_with_slope(player, obj, has_slope);
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

    // Player on ground or just left the ground
    if ((player->time_since_ground < 0.05f) && (frame_counter & 0b11) == 0) {
        particle_templates[CUBE_DRAG].angle = (player->upside_down ? -90 : 90);
        particle_templates[CUBE_DRAG].gravity_y = (player->upside_down ? 300 : -300);
        spawn_particle(CUBE_DRAG, getLeft(player) + 4, (player->upside_down ? getTop(player) - 2 : getBottom(player) + 2), NULL);
    }

    SlopeData slope_data = player->slope_data;

    // If not currently on slope, look at the last frame
    if (!player->slope_data.slope && player->slope_slide_coyote_time) {
        slope_data = player->coyote_slope;
    }
    if ((slope_data.slope || player->on_ground) && (state.input.holdJump)) {
        if (slope_data.slope) {
            // Slope jump
            int orient = grav_slope_orient(slope_data.slope, player);
            if (orient == ORIENT_NORMAL_UP || orient == ORIENT_UD_UP) {
                float time = clampf(10 * (player->timeElapsed - slope_data.elapsed), 0.4f, 1.0f);
                set_p_velocity(player, 0.25f * time * slopeHeights[state.speed] + cube_jump_heights[state.speed]);
            } else {
                set_p_velocity(player, cube_jump_heights[state.speed]);
            }
        } else {
            // Normal jump
            set_p_velocity(player, cube_jump_heights[state.speed]);
        }
        player->inverse_rotation = FALSE;

        player->on_ground = FALSE;
        player->buffering_state = BUFFER_END;

        if (!state.input.pressedJump) {
            // This simulates the holding jump
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
        if (state.input.holdJump) {
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
    if (player->snap_rotation) {
        player->rotation = angle_rad;
    } else {
        player->rotation = iSlerp(player->rotation, RadToDeg(angle_rad), 0.05f, STEPS_DT);
    }
}

void ship_gamemode(Player *player) {
    if (state.dual) {
        // Make both dual players symetric
        if (state.input.holdJump) {
            player->buffering_state = BUFFER_END;
            if (player->vel_y <= 101.541492f)
                player->gravity = player->mini ? 1643.5872f : 1397.0491f;
            else
                player->gravity = player->mini ? 1314.86976f : 1117.64328f;
        } else {
            if (player->vel_y >= 101.541492f)
                player->gravity = player->mini ? -1577.85408f : -1341.1719f;
            else
                player->gravity = player->mini ? -1051.8984f : -894.11464f;
        }
    } else {
        if (state.input.holdJump) {
            player->buffering_state = BUFFER_END;
            if (player->vel_y <= grav(player, 101.541492f))
                player->gravity = player->mini ? 1643.5872f : 1397.0491f;
            else
                player->gravity = player->mini ? 1314.86976f : 1117.64328f;
        } else {
            if (player->vel_y >= grav(player, 101.541492f))
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

static float ballJumpHeights[] = {
    -172.044007,
    -181.11601,
    -185.00401,
    -181.92601
};

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
    if ((state.input.holdJump) && (player->on_ground || player->on_ceiling || player->slope_data.slope) && player->buffering_state == BUFFER_READY) {
        player->upside_down ^= 1;
        set_p_velocity(player, ballJumpHeights[state.speed]);

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
        if (state.input.pressedJump) {
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
    bool buffering_check = ((state.old_player.gamemode == GAMEMODE_SHIP || state.old_player.gamemode == GAMEMODE_CUBE) && (state.input.holdJump));
    if (player->buffering_state == BUFFER_READY && (state.input.pressedJump || buffering_check)) {
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
    if (player->cutscene_timer == 0) wave_trail.opacity = 1.f;

    if (player->buffering_state == BUFFER_READY) player->buffering_state = BUFFER_END;

    bool input = (state.input.holdJump);
    player->gravity = 0;

    player->vel_y = (input * 2 - 1) * player_speeds[state.speed] * (player->mini ? 2 : 1);
    if (player->vel_y != state.old_player.vel_y || player->on_ground != state.old_player.on_ground || player->on_ceiling != state.old_player.on_ceiling) {
        MotionTrail_AddWavePoint(&wave_trail);
    }
}

void run_camera() {
    Player *player = &state.player;

    float calc_x = ((player->x - state.camera_x) * SCALE) - widthAdjust;

    float playable_height = state.ceiling_y - state.ground_y;
    float calc_height = 0;

    if (state.player.gamemode != GAMEMODE_CUBE || state.dual) {
        calc_height = (SCREEN_HEIGHT_AREA - playable_height) / 2;
    }

    state.ground_y_gfx = ease_out(state.ground_y_gfx, calc_height, 0.02f);

    if (level_info.wall_y == 0) {
        if (state.camera_x + WIDTH_ADJUST_AREA + SCREEN_WIDTH_AREA >= level_info.wall_x - (4.5f * 30.f)) {
            level_info.wall_y = MAX(state.camera_y, -30) + (SCREEN_HEIGHT_AREA / 2);
        }
    }

    float camera_x_right = state.camera_x + WIDTH_ADJUST_AREA + SCREEN_WIDTH_AREA;

    if (calc_x >= get_camera_x_scroll_pos()) {
        // Cap at camera_x
        if (level_info.wall_y > 0 && (camera_x_right >= level_info.wall_x - CAMERA_X_WALL_OFFSET)) {
            if (state.camera_wall_timer == 0) {
                state.background_wall_initial_x = state.background_x;
                state.ground_wall_initial_x = state.ground_x;
            }
            state.background_x = easeValue(EASE_IN_OUT, state.background_wall_initial_x, state.background_wall_initial_x + CAMERA_X_WALL_OFFSET * state.mirror_speed_factor, state.camera_wall_timer, 1.f, 2.0f);            
            state.ground_x = easeValue(EASE_IN_OUT, state.ground_wall_initial_x, state.ground_wall_initial_x + CAMERA_X_WALL_OFFSET * state.mirror_speed_factor, state.camera_wall_timer, 1.f, 2.0f);            
        } else {
            state.camera_x += player->vel_x * STEPS_DT;
            state.ground_x += player->vel_x * STEPS_DT * state.mirror_speed_factor;
            state.background_x += player->vel_x * STEPS_DT * state.mirror_speed_factor;
        }
    }

    if (level_info.wall_y > 0 && (camera_x_right >= level_info.wall_x - CAMERA_X_WALL_OFFSET)) {
        if (state.camera_wall_timer == 0) {
            state.camera_wall_initial_y = state.camera_y;
        }

        float final_camera_x_wall = level_info.wall_x - (SCREEN_WIDTH_AREA + WIDTH_ADJUST_AREA);
        float final_camera_y_wall = level_info.wall_y - (SCREEN_HEIGHT_AREA / 2);   

        state.camera_x = easeValue(EASE_IN_OUT, final_camera_x_wall - CAMERA_X_WALL_OFFSET, final_camera_x_wall, state.camera_wall_timer, CAMERA_WALL_ANIM_DURATION, 2.0f);
        state.camera_y = easeValue(EASE_IN_OUT, state.camera_wall_initial_y, final_camera_y_wall, state.camera_wall_timer, CAMERA_WALL_ANIM_DURATION, 2.0f);
        state.camera_wall_timer += STEPS_DT;
        if (completion_shake) {
            state.camera_x = final_camera_x_wall + 3.f * random_float(-1, 1);
            state.camera_y = final_camera_y_wall + 3.f * random_float(-1, 1);
        }
    } else if (player->gamemode == GAMEMODE_CUBE && !state.dual) {
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
        spawn_particle(GLITTER_EFFECT, state.camera_x + WIDTH_ADJUST_AREA + SCREEN_WIDTH_AREA / 2, state.camera_y + (SCREEN_HEIGHT_AREA / 2), NULL);
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

    if (player->gamemode != GAMEMODE_WAVE || player->cutscene_timer > 0) {
        if (wave_trail.opacity > 0) wave_trail.opacity -= 0.02f;
        
        if (wave_trail.opacity <= 0) {
            wave_trail.opacity = 0;
            wave_trail.nuPoints = 0;
        }
    }

    if (player->cutscene_timer > 0) return;

    player->rotation = normalize_angle(player->rotation);
    
    if (player->snap_rotation) {
        player->lerp_rotation = player->rotation;
    } else {
        if (player->gamemode == GAMEMODE_UFO) {
            if (player->slope_data.slope) {
                player->lerp_rotation = iSlerp(player->lerp_rotation, player->rotation, 0.05f, STEPS_DT);
            } else {
                player->lerp_rotation = iSlerp(player->lerp_rotation, player->rotation, 0.1f, STEPS_DT);
            }
        } else {
            player->lerp_rotation = iSlerp(player->lerp_rotation, player->rotation, 0.2f, STEPS_DT);
        }
    }
    
    player->vel_x = player_speeds[state.speed];
    player->vel_y += player->gravity * STEPS_DT;
    player->y += player_get_vel(player, player->vel_y) * STEPS_DT;
    
    player->x += player->vel_x * STEPS_DT;

    player->left_ground = FALSE;

    if (player->ceiling_inv_time > 0) {
        player->ceiling_inv_time -= STEPS_DT;
    } else {
        player->ceiling_inv_time = 0;
    }
    
    bool slopeCheck = player->slope_data.slope && (grav_slope_orient(player->slope_data.slope, player) == ORIENT_NORMAL_DOWN || grav_slope_orient(player->slope_data.slope, player) == ORIENT_UD_DOWN);

    if (getGroundBottom(player) < state.ground_y) {
        if (player->ceiling_inv_time <= 0 && player->gamemode == GAMEMODE_CUBE && player->upside_down) {
            state.dead = TRUE;
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
        }

        if (slopeCheck) {
            clear_slope_data(player);
        }
        
        if (player->gamemode != GAMEMODE_WAVE) player->vel_y = 0;
        player->y = state.ceiling_y - (player->height / 2) - ((player->gamemode == GAMEMODE_WAVE) ? (player->mini ? 3 : 5) : 0);;
    } 
    
    if (player->slope_slide_coyote_time) {
        player->slope_slide_coyote_time--;
        if (!player->slope_slide_coyote_time) {
            player->coyote_slope.slope = NULL;
            player->coyote_slope.elapsed = 0;
            player->coyote_slope.snapDown = FALSE;
        }
    }

    if (player->slope_data.slope) {
        slope_calc(player->slope_data.slope, player);
    }
    
    if (player->gamemode == GAMEMODE_SHIP || player->gamemode == GAMEMODE_WAVE) update_ship_rotation(player);

    player->snap_rotation = FALSE;
}

void anim_player_to_wall(Player *player) {
    float t = CLAMP(easeValue(QUAD_IN, 0, 1, player->cutscene_timer, END_ANIMATION_TIME, 0), 0, 1);

    // (1 - t) and powers
    float one_minus_t = 1.0f - t;
    float one_minus_t_squared = one_minus_t * one_minus_t;
    float t_squared = t * t;

    // Final destination point (offscreen to the right, mid-screen vertically)
    float final_x = level_info.wall_x + 30.f;
    float final_y = level_info.wall_y - 15.f;

    // Control point (slightly above and to the right of starting point)
    float height_diff = fabsf(player->cutscene_initial_player_y - (level_info.wall_y + (SCREEN_HEIGHT_AREA / 2)));
    float offset = height_diff * 0.5f;

    float top_x = level_info.wall_x - (END_ANIMATION_X_START * (2.f / 3));
    float top_y = level_info.wall_y + offset;

    // Start point
    float start_x = player->cutscene_initial_player_x;
    float start_y = player->cutscene_initial_player_y;

    // Quadratic Bézier interpolation
    player->x = 
        one_minus_t_squared * start_x +
        2.0f * one_minus_t * t * top_x +
        t_squared * final_x;

    player->y = 
        one_minus_t_squared * start_y +
        2.0f * one_minus_t * t * top_y +
        t_squared * final_y;
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
    
    if (state.input.holdJump) {
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
    if (player->cutscene_timer == 0) collide_with_objects(player);
    u32 t1 = gettime();
    collision_time = ticks_to_microsecs(t1 - t0) / 1000.f * 4.f;
    
    if (state.noclip) state.dead = FALSE;
    
    if (state.dead) return;

    t0 = gettime();

    if (player->x >= level_info.wall_x - END_ANIMATION_X_START) {
        p1_trail = TRUE;
        if (player->cutscene_timer == 0) {
            // Add a trail point for wave
            if (player->gamemode == GAMEMODE_WAVE) MotionTrail_AddWavePoint(&wave_trail);

            player->cutscene_initial_player_x = player->x;
            player->cutscene_initial_player_y = player->y;
        }
        anim_player_to_wall(player);
        player->lerp_rotation += easeValue(EASE_IN, 0, 415.3848f, player->cutscene_timer, 0.5f, 2.f) * STEPS_DT;
        player->rotation = player->lerp_rotation;
        player->cutscene_timer += STEPS_DT;
        
        // End level
        if (player->x > level_info.wall_x) {
            level_info.completing = TRUE;
        }
    } 
    
    run_player(player);
    
    t1 = gettime();
    player_time = ticks_to_microsecs(t1 - t0) / 1000.f * 4.f;
    
    if (state.noclip) state.dead = FALSE;
    
    do_ball_reflection();

    if (state.hitbox_display == 2) add_new_hitbox(player);

}

void set_camera_x(float x) {
    state.camera_x = x;
    state.camera_x_lerp = x;
}

float get_camera_x_scroll_pos() {
    float factor_x;
    if (screenWidth <= 640)
        factor_x = 1.0f;
    else
        factor_x = 1.0f + 0.00535714f * (screenWidth - 640);
    return 120 * factor_x;
}

void full_init_variables() {
    state.ground_x = 0;
    state.background_x = 0;

    set_particle_color(GLITTER_EFFECT, p1.r, p1.g, p1.b);
    set_particle_color(END_WALL_PARTICLES, p1.r, p1.g, p1.b);
    set_particle_color(END_WALL_COLL_CIRCLE, p1.r, p1.g, p1.b);
    set_particle_color(END_WALL_COLL_CIRCUNFERENCE, p1.r, p1.g, p1.b);
    set_particle_color(END_WALL_COMPLETE_CIRCLES, p2.r, p2.g, p2.b);
    set_particle_color(END_WALL_FIREWORK, p1.r, p1.g, p1.b);
    init_variables();
}

void init_variables() {
    MotionTrail_Init(&trail_p1, 0.3f, 3, 10.0f, FALSE, p2, trail_tex);
    MotionTrail_Init(&trail_p2, 0.3f, 3, 10.0f, FALSE, p1, trail_tex);
    MotionTrail_Init(&wave_trail_p1, 3.f, 3, 10.0f, TRUE, p2, trail_tex);
    MotionTrail_Init(&wave_trail_p2, 3.f, 3, 10.0f, TRUE, p1, trail_tex);
    MotionTrail_StopStroke(&trail_p1);
    MotionTrail_StopStroke(&trail_p2);

    set_camera_x(-get_camera_x_scroll_pos());
    state.camera_wall_timer = 0;
    state.camera_wall_initial_y = 0;

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
    state.mirror_mult = 1;

    Player *player = &state.player;
    player->cutscene_timer = 0;
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

    player->cutscene_initial_player_x = 0;
    player->cutscene_initial_player_y = 0;

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
            break;
        case GAMEMODE_CUBE:
            state.camera_intended_y = -95.f;
    }
    
    if (level_info.initial_dual) {
        state.dual = TRUE;
        state.dual_portal_y = 0.f;
        setup_dual();
    }

    // Set camera vertical pos
    state.camera_y = state.camera_intended_y;
    state.camera_y_lerp = state.camera_y;
    state.intermediate_camera_y = state.camera_y;

    float playable_height = state.ceiling_y - state.ground_y;
    float calc_height = 0;

    if (state.player.gamemode != GAMEMODE_CUBE || state.dual) {
        calc_height = (SCREEN_HEIGHT_AREA - playable_height) / 2;
    }

    state.ground_y_gfx = calc_height;

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
    int orient = obj->object.orientation;

    if (player->upside_down) {
        // Flip vertically slope orientation
        if (orient == ORIENT_UD_UP)
			orient = ORIENT_NORMAL_UP;
		else if (orient == ORIENT_UD_DOWN)
			orient = ORIENT_NORMAL_DOWN;
		else if (orient == ORIENT_NORMAL_UP)
			orient = ORIENT_UD_UP;
		else if (orient == ORIENT_NORMAL_DOWN)
			orient = ORIENT_UD_DOWN;
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
    if (orient == ORIENT_NORMAL_DOWN || orient == ORIENT_UD_DOWN) {
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
    int orient = obj->object.orientation;

    if (orient == ORIENT_NORMAL_UP) angle = -fabsf(angle);
    if (orient == ORIENT_NORMAL_DOWN) angle = fabsf(angle);

    return angle;
}

float expected_slope_y(GameObject *obj, Player *player) {
    int flipping = grav_slope_orient(obj, player) >= 2;
    int mult = (player->upside_down ^ flipping) ? -1 : 1;
    
    float angle = slope_angle(obj, player);
    float ydist = mult * player->height * sqrtf(powf(tanf(angle), 2) + 1) / 2;
    float pos_relative = ((float) obj->height / obj->width) * (player->x - obj_getLeft(obj));

    // Get correct slope y depending on combination of player gravity and slope orientation
    if ((angle > 0) ^ player->upside_down ^ flipping) {
        return obj_getBottom(obj) + MIN(pos_relative + ydist, obj->height + player->height / 2);
    } else {
        return obj_getTop(obj) - MAX(pos_relative - ydist, -player->height / 2);
    }
}

void slope_snap_y(GameObject *obj, Player *player) {
    int orientation = grav_slope_orient(obj, player);

    switch (orientation) {
        case ORIENT_NORMAL_UP: // Normal - up
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
            break;
        case ORIENT_NORMAL_DOWN: // Normal - down
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
            break;
        case ORIENT_UD_DOWN: // Upside down - down
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
            break;
        case ORIENT_UD_UP: // Upside down - up
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
            break;
    }
}

void slope_calc(GameObject *obj, Player *player) {
    int orientation = grav_slope_orient(obj, player);
    if (orientation == ORIENT_NORMAL_UP) { // Normal - up
        // Handle leaving slope
        if (!slope_touching(obj, player)) {
            clear_slope_data(player);
            return;
        }

        // On slope
        if (gravBottom(player) != obj_gravTop(player, obj)) {
            slope_snap_y(obj, player);
        }

        // Sliding off slope
        if (gravBottom(player) >= obj_gravTop(player, obj)) {
            float vel = 0.9f * MIN(1.12 / slope_angle(obj, player), 1.54) * (obj->height * player_speeds[state.speed] / obj->width);
            float time = clampf(10 * (player->timeElapsed - player->slope_data.elapsed), 0.4f, 1.0f);
            
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

            //printf("%d - vel %.2f orig %.2f time %.2f elapsed %.2f %.2f y %.2f obj_y %.2f\n", state.current_player, -vel, -orig, time, player->timeElapsed, player->slope_data.elapsed, player->y, obj->y);
            player->vel_y = vel;
            player->inverse_rotation = TRUE;
            player->coyote_slope = player->slope_data;
            player->slope_slide_coyote_time = 2;
            clear_slope_data(player);
        }
    } else if (orientation == ORIENT_NORMAL_DOWN) { // Normal - down
        // Handle leaving slope
        if (player->vel_y > 0) {
            clear_slope_data(player);
            return;
        }

        if (gravBottom(player) != obj_gravTop(player, obj) || player->slope_data.snapDown) {
            slope_snap_y(obj, player);
        }

        if (obj_gravTop(player, obj) <= grav(player, player->y) || getLeft(player) - obj_getRight(obj) > 0) {
            float vel = -falls[state.speed] * ((float) obj->height / obj->width);
            player->vel_y = vel;
            clear_slope_data(player);
        }
    } else if (orientation == ORIENT_UD_UP) { // Upside down - up
        // Handle leaving slope
        if (!slope_touching(obj, player)) {
            clear_slope_data(player);
            return;
        }
        
        bool gravSnap = (player->ceiling_inv_time > 0) || (player->gravObj && player->gravObj->hitbox_counter[state.current_player] == 1);
        
        if (player->gamemode == GAMEMODE_CUBE && !gravSnap) {
            state.dead = TRUE;
        }

        // On slope
        if (gravBottom(player) != obj_gravTop(player, obj)) {
            slope_snap_y(obj, player);
        }

        // Sliding off slope
        if (gravTop(player) <= obj_gravBottom(player, obj)) {
            float vel = 0.9f * MIN(1.12 / slope_angle(obj, player), 1.54) * (obj->height * player_speeds[state.speed] / obj->width);
            float time = clampf(10 * (player->timeElapsed - player->slope_data.elapsed), 0.4f, 1.0f);
            
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
            //printf("%d - vel %.2f orig %.2f time %.2f elapsed %.2f %.2f y %.2f obj_y %.2f\n", state.current_player, -vel, -orig, time, player->timeElapsed, player->slope_data.elapsed, player->y, obj->y);

            player->inverse_rotation = TRUE;
            player->coyote_slope = player->slope_data;
            player->slope_slide_coyote_time = 2;
            clear_slope_data(player);
        }
    } else if (orientation == ORIENT_UD_DOWN) { // Upside down - down
        // Handle leaving slope
        if (player->vel_y < 0) {
            clear_slope_data(player);
            return;
        }
        
        bool gravSnap = (player->ceiling_inv_time > 0) || (player->gravObj && player->gravObj->hitbox_counter[state.current_player] == 1);
        
        if (player->gamemode == GAMEMODE_CUBE && !gravSnap) {
            state.dead = TRUE;
        }

        // On slope
        if (gravTop(player) != obj_gravBottom(player, obj) || player->slope_data.snapDown) {
            slope_snap_y(obj, player);
        }

        // Sliding off
        if (obj_gravTop(player, obj) <= grav(player, player->y) || getLeft(player) - obj_getRight(obj) > 0) {
            float vel = falls[state.speed] * ((float) obj->height / obj->width);
            player->vel_y = vel;
            clear_slope_data(player);
        }
    }
}


bool player_circle_touches_slope(GameObject *obj, Player *player) {
    float x1, y1, x2, y2;
    int orientation = obj->object.orientation;

    float hw = obj->width / 2.f, hh = obj->height / 2.f;

    // Collide with hipotenuse
    switch (orientation) {
        case ORIENT_NORMAL_UP:
        case ORIENT_UD_DOWN:
            x1 = obj->x - hw;
            y1 = obj->y - hh;
            x2 = obj->x + hw;
            y2 = obj->y + hh;
            break;
        case ORIENT_NORMAL_DOWN:
        case ORIENT_UD_UP:
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
        case ORIENT_NORMAL_UP:
        case ORIENT_UD_UP:
            x1 = obj->x + hw;
            y1 = obj->y - hh;
            x2 = obj->x + hw;
            y2 = obj->y + hh;
            break;
        case ORIENT_NORMAL_DOWN:
        case ORIENT_UD_DOWN:
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
        case ORIENT_NORMAL_UP:
        case ORIENT_NORMAL_DOWN:
            x1 = obj->x + hw;
            y1 = obj->y - hh;
            x2 = obj->x - hw;
            y2 = obj->y - hh;
            break;
        case ORIENT_UD_DOWN:
        case ORIENT_UD_UP:
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
    if (orient == ORIENT_NORMAL_UP || orient == ORIENT_UD_UP) {
        bool internalCollidingSlope = intersect(
            player->x, player->y, internal.width, internal.height, 0, 
            obj_getRight(obj), obj->y, 1, obj->height, 0
        );

        // Die if so
        if (internalCollidingSlope) state.dead = TRUE;
    }

    // Normal slope - resting on bottom
    if (
        !state.old_player.slope_data.slope &&
        orient < ORIENT_UD_DOWN && 
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
        orient >= ORIENT_UD_DOWN && 
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
        (orient == ORIENT_NORMAL_DOWN || orient == ORIENT_UD_DOWN) && 
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
            if (orient == ORIENT_NORMAL_DOWN) {
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
                // Check if going from going down to up
                grav_slope_orient(slope, player) != grav_slope_orient(obj, player) && 
                (
                    (grav_slope_orient(slope, player) == ORIENT_NORMAL_DOWN && grav_slope_orient(obj, player) == ORIENT_NORMAL_UP) ||
                    (grav_slope_orient(slope, player) == ORIENT_UD_DOWN && grav_slope_orient(obj, player) == ORIENT_UD_UP)
                )
            ) 
        ) && slope_touching(obj, player) && colliding && obj_gravTop(player, obj) - gravBottom(player) > 1
    ) {
        
        if (slope && slope_angle(obj, player) < slope_angle(slope, player)) return;
        float angle = atanf((player->vel_y * STEPS_DT) / (player_speeds[state.speed] * STEPS_DT));
        
        if (orient >= ORIENT_UD_DOWN) angle = -angle;

        bool hasSlope = state.old_player.slope_data.slope;

        // Check if the old slope and this slope have the same orientation, if not, then the player doesn't have an slope
        if (hasSlope && slope) {
            hasSlope = state.old_player.slope_data.slope->object.orientation == slope->object.orientation;
        }
        

        #define SLOPE_EPSILON 3
        bool projectedHit = (orient == ORIENT_NORMAL_DOWN || orient == ORIENT_UD_DOWN) ? (angle * 1.1f <= slope_angle(obj, player)) : (angle <= slope_angle(obj, player));
        bool clip = slope_touching(obj, player);
        bool snapDown = (orient == ORIENT_NORMAL_DOWN || orient == ORIENT_UD_DOWN) && player->vel_y * mult > 0 && player->x - obj_getLeft(obj) > 0;

        //printf("p %d - orient %d, slope angle %.2f - hasSlope %d, projectedHit %d clip %d snapDown %d (clip val %.2f)\n", state.current_player, orient, slope_angle(obj,player), hasSlope, projectedHit, clip, snapDown, grav(player, player->y) - grav(player, expected_slope_y(obj, player)));
        
        if ((projectedHit && clip) || snapDown) {
            // If wave, just die, nothing else, wave hates slopes
            if (player->gamemode == GAMEMODE_WAVE) {
                state.dead = TRUE;
                return;
            }
            
            player->on_ground = TRUE;
            player->inverse_rotation = FALSE;
            player->slope_data.slope = obj;
            slope_snap_y(obj, player);
            snap_player_to_slope(obj, player);

            if (is_spike_slope(obj)) {
                state.dead = TRUE;
            }

            // If player is on an slope that goes down, and is in the top corner, snap down
            if (snapDown && !hasSlope) {
                if (orient == ORIENT_NORMAL_DOWN && player->vel_y <= 0) {
                    player->y = grav(player, obj_gravTop(player, obj)) + grav(player, player->height / 2);
                    player->slope_data.snapDown = TRUE;
                    player->vel_y = 0;
                } else if (orient == ORIENT_UD_DOWN && player->vel_y >= 0) {
                    player->y = grav(player, obj_gravBottom(player, obj)) - grav(player, player->height / 2);
                    player->slope_data.snapDown = TRUE;
                    player->vel_y = 0;
                }
            }

            // If the player wasn't on an slope, initialize the time elapsed
            if (!player->slope_data.elapsed) {
                Player *other_player = (state.current_player == 0 ? &state.player2 : &state.player);

                // Make both times synced if close enough
                if (other_player->slope_data.slope && fabsf(player->timeElapsed - other_player->slope_data.elapsed) < 0.10) {
                    player->slope_data.elapsed = other_player->slope_data.elapsed;
                    //printf("yes elapsing %.2f\n", other_player->slope_data.elapsed);
                } else {
                    //printf("no elapsing %.2f becoz other player slope %d and fabsf = %.2f\n", player->timeElapsed, (int) other_player->slope_data.slope, fabsf(player->timeElapsed - other_player->slope_data.elapsed));
                    player->slope_data.elapsed = player->timeElapsed;
                }
            }
        } 
    }
}

bool slope_touching(GameObject *obj, Player *player) {
    bool hasSlope = player->slope_data.slope;

    // If player is on slope, add a bit of hitbox
    if (hasSlope) {
        int mult = grav_slope_orient(player->slope_data.slope, player) >= 2 ? -1 : 1;
        hasSlope = hasSlope && player->vel_y * mult <= 0;
    }
    float deg = RadToDeg(fabsf(slope_angle(obj, player)));
    float snap_height = 20 * (deg / 45);
    float min = hasSlope ? -3 : 0;

    switch (grav_slope_orient(obj, player)) {
        case ORIENT_NORMAL_UP:
        case ORIENT_NORMAL_DOWN:
            float diff = grav(player, expected_slope_y(obj, player)) - grav(player, player->y);
            return diff >= min && diff <= snap_height;
        case ORIENT_UD_UP:
        case ORIENT_UD_DOWN:
            float diff_ud = grav(player, player->y) - grav(player, expected_slope_y(obj, player));  
            return diff_ud >= min && diff_ud <= snap_height;
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
        tri[ti].x = calc_x_on_screen(rect[i].x);
        tri[ti++].y = calc_y_on_screen(rect[i].y);
    }

    draw_polygon_inward_mitered(tri, 3, 2.f, color);
}

void draw_square(Vec2D rect[4], uint32_t color) {
    Vec2D center = {
        (rect[0].x + rect[1].x + rect[2].x + rect[3].x) / 4.f,
        (rect[0].y + rect[1].y + rect[2].y + rect[3].y) / 4.f
    };

    for (int i = 0; i < 4; i++) {
        int j = (i + 1) % 4;

        draw_hitbox_line_inward(rect,
            calc_x_on_screen(rect[i].x), calc_y_on_screen(rect[i].y),
            calc_x_on_screen(rect[j].x), calc_y_on_screen(rect[j].y),
            2.0f, center.x, center.y, color
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

        draw_triangle_from_rect(rect, 3 - obj->object.orientation,color);
    } else if (objects[obj->id].is_saw) {
        if (hitbox.radius == 0) return;

        float calc_radius = hitbox.radius * SCALE;

        custom_circunference(calc_x_on_screen(x), calc_y_on_screen(y), calc_radius, color, 2.f);
    } else {
        if (w == 0 || h == 0) return;

        get_corners(x, y, w, h, angle, rect);
        draw_square(rect, color);
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

    draw_square(rect, RGBA(0x7f, 0x00, 0x00, 0xff));

    // Internal hitbox
    get_corners(player->x, player->y, internal.width, internal.height, 0, rect);

    draw_square(rect, RGBA(0x00, 0x00, 0x7f, 0xff));

    // Circle hitbox
    float calc_radius = (player->width / 2) * SCALE;
    custom_circunference(calc_x_on_screen(player->x), calc_y_on_screen(player->y), calc_radius, RGBA(0xff, 0x00, 0x00, 0xff), 2.f);

    // Unrotated hitbox
    get_corners(player->x, player->y, player->width, player->height, 0, rect);

    draw_square(rect, RGBA(0xff, 0x00, 0x00, 0xff));
}