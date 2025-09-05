#pragma once

#include "player.h"
#include "particles.h"

extern int screenWidth;
extern int widthAdjust;
extern float screen_factor_x;
extern float screen_factor_y;
extern int screenHeight;

#define HITBOX_TRAIL_SIZE 64

typedef struct {
    u8 pressedA:1;
    u8 holdA:1;

    u8 pressedB:1;
    u8 holdB:1;

    u8 pressedHome:1;

    u8 pressed1orX:1;
    u8 hold1orX:1;
    
    u8 pressed2orY:1;
    u8 hold2orY:1;
    
    u8 pressedPlusOrL:1;
    u8 holdPlusOrL:1;
    
    u8 pressedMinusOrR:1;
    u8 holdMinusOrR:1;

    u8 pressedJump:1;
    u8 holdJump:1;

    u8 pressedDir;
    u8 holdDir;
} KeyInput;

typedef struct {
    float x;
    float y;
    float width;
    float height;
    float rotation;
    InternalHitbox internal_hitbox;
} PlayerHitboxTrail;

typedef struct {
    float camera_x;
    float camera_y;

    float camera_intended_y;

    float camera_x_lerp;
    float camera_y_lerp;

    float intermediate_camera_y;
    
    float camera_wall_timer;
    float camera_wall_initial_y;

    float ground_x;
    float ground_wall_initial_x;

    float background_x;
    float background_wall_initial_x;
    
    Player player;
    Player player2;

    unsigned char current_player;

    Player old_player;

    int mirror_mult;
    float intended_mirror_factor;
    float intended_mirror_speed_factor;
    float mirror_speed_factor;
    float mirror_factor;

    float ground_y;
    float ceiling_y;
    float ground_y_gfx;

    float dual_portal_y;
    unsigned char speed;

    bool noclip;
    int hitbox_display;
    bool dual;
    bool dual_setup;
    bool dead;

    bool paused;

    float level_progress;
    
    Particle particles[MAX_PARTICLES];

    int last_hitbox_trail;
    PlayerHitboxTrail hitbox_trail_players[2][HITBOX_TRAIL_SIZE];

    KeyInput input;
} GameState;

enum GameRoutine {
    ROUTINE_MENU,
    ROUTINE_GAME
};

#define INPUT_UP    PAD_BUTTON_UP
#define INPUT_DOWN  PAD_BUTTON_DOWN
#define INPUT_RIGHT PAD_BUTTON_RIGHT
#define INPUT_LEFT  PAD_BUTTON_LEFT

#define SDCARD_FOLDER "sd:/apps/wiidash"
#define RESOURCES_FOLDER "resources"
#define SONGS_FOLDER "songs"
#define USER_SONGS_FOLDER "user_songs"
#define USER_LEVELS_FOLDER "user_levels"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define CLAMP(x, lower, upper) (MIN((upper), MAX((x), (lower))))

extern int gameRoutine;

extern int frame_counter;

extern float physics_time;
extern float collision_time;
extern float particles_time;
extern float triggers_time;
extern float player_time;

extern float draw_time;
extern float layer_sorting;
extern float obj_particles_time;
extern float player_draw_time;

extern int number_of_collisions;
extern int number_of_collisions_checks;

extern char launch_dir[256];

extern int frameCount;

extern GameState state;

extern GRRLIB_texImg *big_font_text;
extern GRRLIB_texImg *font;
extern GRRLIB_texImg *cursor;

extern float ir_x;
extern float ir_y;

extern float dt;

void draw_game();
void update_input();
void update_external_input(KeyInput *input);
