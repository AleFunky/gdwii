#pragma once

#include <stdbool.h>
#include <grrlib.h>
#include "level_loading.h"
#include "math.h"

#define SCREEN_WIDTH_AREA ((screenWidth / BLOCK_SIZE_PX) * 30) 
#define WIDTH_ADJUST_AREA ((widthAdjust / BLOCK_SIZE_PX) * 30)
#define SCREEN_HEIGHT_AREA ((screenHeight / BLOCK_SIZE_PX) * 30 + 8) 

#define CAMERA_X_OFFSET (WIDTH_ADJUST_AREA - 4)
#define CAMERA_X_WALL_OFFSET (2 * 30.F)
#define CAMERA_WALL_ANIM_DURATION 1.f

#define MAX_LEVEL_HEIGHT 2400.f

#define MAX_COLLIDED_OBJECTS 4096


#define END_ANIMATION_TIME 1.f
#define END_ANIMATION_X_START (9.f * 30.f)

typedef struct {
    GameObject *slope;
    float elapsed;
    bool snapDown;
} SlopeData;

typedef struct {
    float width;
    float height;
} InternalHitbox;

typedef struct {
    float x;
    float y;
    
    float vel_x;
    float vel_y;
    
    float gravity;

    float rotation;
    float lerp_rotation;
    
    float width;
    float height;

    InternalHitbox internal_hitbox;

    int gamemode;

    bool on_ground;
    bool on_ceiling;
    bool mini;
    bool upside_down;
    bool touching_slope;
    bool inverse_rotation;
    bool snap_rotation;
    
    GameObject *potentialSlope;

    bool left_ground;

    float ball_rotation_speed;

    float cutscene_timer;

    int buffering_state;

    float time_since_ground;
    
    float ufo_last_y;

    float ceiling_inv_time;

    float timeElapsed;

    GameObject *gravObj;

    float cutscene_initial_player_x;
    float cutscene_initial_player_y;

    int slope_slide_coyote_time;

    SlopeData coyote_slope;

    SlopeData slope_data;
} Player;

enum BufferingState {
    BUFFER_NONE,
    BUFFER_READY,
    BUFFER_END
};

enum PlayerGamemode {
    GAMEMODE_CUBE,
    GAMEMODE_SHIP,
    GAMEMODE_BALL,
    GAMEMODE_UFO,
    GAMEMODE_WAVE,
    GAMEMODE_COUNT
};

enum PlayerSpeeds {
    SPEED_SLOW,
    SPEED_NORMAL,
    SPEED_FAST,
    SPEED_FASTER,
    SPEED_COUNT
};

enum SlopeOrientations {
    ORIENT_NORMAL_UP,
    ORIENT_NORMAL_DOWN,
    ORIENT_UD_DOWN,
    ORIENT_UD_UP
};

extern Color p1;
extern Color p2;

extern const float player_speeds[SPEED_COUNT];

extern GRRLIB_texImg *trail_tex;

void anim_player_to_wall(Player *player);
void handle_death();

void set_camera_x(float x);
void init_variables();
void full_init_variables();
float get_camera_x_scroll_pos();

void load_icons();
void unload_icons();
void draw_player(Player *player);
void handle_mirror_transition();
void handle_player(Player *player);
void set_p_velocity(Player *player, float vel);
GRRLIB_texImg *get_p1_trail_tex();
float slope_angle(GameObject *obj, Player *player);
void slope_collide(GameObject *obj, Player *player);
void slope_calc(GameObject *obj, Player *player);
void clear_slope_data(Player *player);
int grav_slope_orient(GameObject *obj, Player *player);
void snap_player_to_slope(GameObject *obj, Player *player); 
bool slope_touching(GameObject *obj, Player *player);
void run_camera();
float get_slope_angle(GameObject *obj);

inline float getTop(Player *player)  { return player->y + player->height / 2; }
inline float getBottom(Player *player)  { return player->y - player->height / 2; }

inline float getGroundTop(Player *player)  { return player->y + (player->height / 2) + ((player->gamemode == GAMEMODE_WAVE) ? (player->mini ? 3 : 5) : 0); }
inline float getGroundBottom(Player *player)  { return player->y - (player->height / 2) - ((player->gamemode == GAMEMODE_WAVE) ? (player->mini ? 3 : 5) : 0); }

inline float getRight(Player *player)  { return player->x + player->width / 2; }
inline float getLeft(Player *player)  { return player->x - player->width / 2; }

inline float getInternalTop(Player *player)  { return player->y + player->internal_hitbox.height / 2; }
inline float getInternalBottom(Player *player)  { return player->y - player->internal_hitbox.height / 2; }
inline float getInternalRight(Player *player)  { return player->x + player->internal_hitbox.width / 2; }
inline float getInternalLeft(Player *player)  { return player->x - player->internal_hitbox.width / 2; }

inline float gravBottom(Player *player) { return player->upside_down ? -getTop(player) : getBottom(player); }
inline float gravTop(Player *player) { return player->upside_down ? -getBottom(player) : getTop(player); }

inline float gravInternalBottom(Player *player) { return player->upside_down ? -getInternalTop(player) : getInternalBottom(player); }
inline float gravInternalTop(Player *player) { return player->upside_down ? -getInternalBottom(player) : getInternalTop(player); }

inline float grav(Player *player, float val) { return player->upside_down ? -val : val; }

inline float obj_getTop(GameObject *object)  { 
    return object->y + object->height / 2; 
}
inline float obj_getBottom(GameObject *object)  { 
    return object->y - object->height / 2; 
}
inline float obj_getRight(GameObject *object)  {  
    return object->x + object->width / 2; 
}
inline float obj_getLeft(GameObject *object)  { 
    return object->x - object->width / 2; 
}
inline float obj_gravBottom(Player *player, GameObject *object) { return player->upside_down ? -obj_getTop(object) : obj_getBottom(object); }
inline float obj_gravTop(Player *player, GameObject *object) { return player->upside_down ? -obj_getBottom(object) : obj_getTop(object); }

void add_new_hitbox(Player *player);
void draw_hitbox(GameObject *obj);
void draw_player_hitbox(Player *player);
void draw_hitbox_trail(int player);
bool player_circle_touches_slope(GameObject *obj, Player *player);
bool is_spike_slope(GameObject *obj);