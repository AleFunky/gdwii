#include "main.h"
#include <grrlib.h>
#include <stdlib.h>
#include <wiiuse/wpad.h>

#include "level.h"
#include "custom_mp3player.h"
#include "oggplayer.h"
#include "playSound_01_ogg.h"

#include <fat.h>
#include <dirent.h>
#include <sys/types.h>

#include "filesystem.h"
#include "math.h"

#include "menu.h"

#include "game.h"

#include <stdio.h>

#include "font_stuff.h"

GRRLIB_texImg *menu_top_bar;
GRRLIB_texImg *menu_corner_squares;
GRRLIB_texImg *menu_arrow;
GRRLIB_texImg *gradient_texture;
// add ground
GRRLIB_texImg *menu_ground;
GRRLIB_texImg *font_bold;
GRRLIB_texImg *ground_line_texture;

int main_levels();
int sdcard_levels();
void game_folder_not_found();

int level_id = 0;

typedef struct {
    char name[MAX_PATH_LEN];
    bool is_dir;
} FileOrFolder;

FileOrFolder sd_level_paths[MAX_SD_LEVELS];
int sd_level_count = 0;

char current_level_name[255];

int level_mode = 0;

size_t outsize;
#define MAX_BUTTONS 16

typedef struct{
    int x;
    int y;
    int width;
    int height;
    bool show;
    GRRLIB_texImg *texture;
    void (*func)();
    bool flip_x;
    bool flip_y;

}buttons;

buttons button_list[MAX_BUTTONS];
int button_count = 0;

int create_button(int x, int y, int width, int height, bool show, GRRLIB_texImg *worknowplease, void (*func)(), bool flip_x, bool flip_y){
    if (button_count > MAX_BUTTONS) {return -1;}
    button_list[button_count].x = x;
    button_list[button_count].y = y;
    button_list[button_count].width = width;
    button_list[button_count].height = height;
    button_list[button_count].show = show;
    button_list[button_count].texture = worknowplease;
    button_list[button_count].func = func;
    button_list[button_count].flip_x = flip_x;
    button_list[button_count].flip_y = flip_y;
    button_count++;
    return 0;
}

bool exit_menu = false;
int error_code = 0;

#define NUM_LVL_COLORS 20
const int default_lvl_colors[NUM_LVL_COLORS] = {
  RGBA(0, 0, 232, 255), // Stereo Madness
  RGBA(227, 0, 229, 255), // Back on Track
  RGBA(233, 0, 115, 255), // Polargeist
  RGBA(233, 0, 0, 255), // Dry Out
  RGBA(231, 112, 0, 255), // Base After Base
  RGBA(233, 232, 0, 255), // Cant Let Go
  RGBA(0, 231, 0, 255), // Jumper
  RGBA(0, 227, 228, 255), // Time Machine
  RGBA(0, 112, 0xE5, 255), // Cycles
  RGBA(0, 0, 0xE9, 255), // XStep
  RGBA(0xE6, 0, 0xE8, 255), // Clutterfunk
  RGBA(231, 0, 115, 255), // ToE
  RGBA(231, 0, 0, 255), // EA
  RGBA(231, 113, 0, 255), // Clubstep
  RGBA(231, 231, 0, 255), // Electrodynamix
  RGBA(0, 231, 0, 255), // HF
  RGBA(0, 228, 228, 255), // BP
  RGBA(0, 112, 231, 255), // ToE2
  RGBA(0, 0, 0xE9, 255), // Geometrical Dominator
  RGBA(0xE6, 0, 0xE8, 255), // Deadlocked
};

const int default_level_difficulty[LEVEL_NUM] = {
  0, // Stereo Madness | easy
  0, // Back on Track | easy
  1, // Polargeist | normal
  1, // Dry Out | i think you get it now
  2, // Base After Base
  2, // Cant Let Go
  3, // Jumper
  3, // Time Machine
  3, // Cycles
  4, // XStep
  4, // Clutterfunk
  4, // ToE
  4, // EA
  5, // Clubstep
  4, // Electrodynamix
  4, // HF
  3, // BP
  5, // ToE2
  4, // Geometrical Dominator
  5, // Deadlocked
  0,
  5,
  5,
  5,
  6,
};

#define FACES_COUNT 7
GRRLIB_texImg *difficulty_faces[FACES_COUNT] = {};

void menu_go_left(){
    level_id--;
    if (level_id < 0) level_id = LEVEL_NUM - 1;
}

void menu_go_right(){
    level_id++;
    if (level_id >= LEVEL_NUM) level_id = 0;
}

void start_level(){
    gameRoutine = ROUTINE_GAME;
    MP3Player_Stop();
    PlayOgg(playSound_01_ogg, playSound_01_ogg_size, 0, OGG_ONE_TIME);
    for (int i = 0; i < 90; i++) {
        update_input();
        VIDEO_WaitVSync();
    }
    int code = load_level((char *) levels[level_id].data_ptr, FALSE);
    if (!code) {
        exit_menu = true;
    }
}

void game_folder_not_found() {
    char path[256];
    snprintf(path, sizeof(path), "%s", launch_dir);

    while (1) {
        update_input();

        GRRLIB_FillScreen(RGBA(0, 127, 255, 255));

        
        draw_text(big_font, big_font_text, 0, 20, 0.5, "Couldn't find the game folder.");
        draw_text(big_font, big_font_text, 0, 45, 0.5, "Make sure the resources folder");
        draw_text(big_font, big_font_text, 0, 70, 0.5, "and boot.dol is in the following");
        draw_text(big_font, big_font_text, 0, 95, 0.5, "path:");
        draw_text(big_font, big_font_text, 0, 120, 0.5,  path);
        draw_text(big_font, big_font_text, 0, 155, 0.5,  "Press home to exit.");
                
        if (state.input.pressedHome) {
            break;
        }

        GRRLIB_Render();
    }
}

char current_directory[256];
int dir_level = 0;

int custom_song_id = -1;

void print_custom_song(int song_id) {
    char text[269];
    snprintf(text, sizeof(text), "Level uses song: %d.mp3", song_id);
    
    draw_text(big_font, big_font_text, 0, 270, 0.5, text);

    if (!check_song(song_id)) {
        draw_text(big_font, big_font_text, 0, 300, 0.5, "Song not found.");
        draw_text(big_font, big_font_text, 0, 330, 0.5, "Add it in the \"%s\" folder.", USER_SONGS_FOLDER);
    }
}

void check_custom_song(char *level_data) {
    char *gmd_custom_song_id = extract_gmd_key((const char *) level_data, "k45", "i");
    if (gmd_custom_song_id) {
        custom_song_id = atoi(gmd_custom_song_id);        
    } else {
        custom_song_id = -1;
    }
}

void load_folder(char *dir) {
    sd_level_count = 0;

	struct dirent *pent;
	struct stat statbuf;

    char directory[268];
    
    if (strlen(dir) > 0) {
        snprintf(directory, sizeof(directory), "%s", dir);
    } else {
        snprintf(directory, sizeof(directory), "%s/%s", launch_dir, USER_LEVELS_FOLDER);
    }

    strcpy(current_directory, directory);

    DIR *level_dir = opendir(directory);

    printf("Loaded folder: %s\n", directory);

    while ((pent=readdir(level_dir))!=NULL) {
        stat(pent->d_name,&statbuf);

        if(strcmp(".", pent->d_name) == 0 || strcmp("..", pent->d_name) == 0)
            continue;

        const char *ext = strrchr(pent->d_name, '.');
    
        if (ext) {
            if (strcmp(ext, ".gmd") == 0) { // GMD
                snprintf(sd_level_paths[sd_level_count].name, MAX_PATH_LEN, "%s/%s", directory, pent->d_name);
                sd_level_paths[sd_level_count].is_dir = FALSE;
                sd_level_count++;

                if (sd_level_count >= MAX_SD_LEVELS) break;

                printf("Found GMD file: %s %llu\n", pent->d_name, pent->d_stat.st_size);
            }
        } else { { // Folder
            snprintf(sd_level_paths[sd_level_count].name, MAX_PATH_LEN, "%s/%s", directory, pent->d_name);
            sd_level_paths[sd_level_count].is_dir = TRUE;
            sd_level_count++;

            if (sd_level_count >= MAX_SD_LEVELS) break;

            printf("Found folder: %s %llu\n", pent->d_name, pent->d_stat.st_size);
        } }
    }
    closedir(level_dir);
}

void go_back_directory(char *path) {
    size_t len = strlen(path);
    if (len == 0) return;

    while (len > 0 && path[len - 1] == '/') {
        path[--len] = '\0';
    }

    while (len > 0 && path[len - 1] != '/') {
        path[--len] = '\0';
    }

    if (len > 0 && path[len - 1] == '/') {
        path[len - 1] = '\0';
    }
}

void draw_menu() {
    if (level_mode == 0) {
        main_levels();
    } else {
        sdcard_levels();
    }
}

int menu_loop() {
    exit_menu = false;
    if (!fatInitDefault()) {
		printf("fatInitDefault failure\n");
	}
    
    DIR *pdir = opendir(launch_dir);

	if (!pdir){
		game_folder_not_found();
        return TRUE;
	}

    load_folder(current_directory);

	closedir(pdir);
    
    // Read first gmd
    char *level_data = read_file(sd_level_paths[level_id].name, &outsize);
    if (level_data) {
        snprintf(current_level_name, 255, "%s - by %s", get_level_name(level_data), get_author_name(level_data));
        free(level_data);
    }

    size_t size;
    char *menuLoop = load_song("menuLoop.mp3", &size);

    font_bold = GRRLIB_LoadTexturePNG(font_bold_png);
    GRRLIB_InitTileSet(font_bold, 24, 36, 32);

    button_count = 0;

    menu_top_bar = GRRLIB_LoadTexturePNG(top_bar_png);
    menu_corner_squares = GRRLIB_LoadTexturePNG(corner_squares_png);
    menu_arrow = GRRLIB_LoadTexturePNG(arrow_png);
    gradient_texture = GRRLIB_LoadTexturePNG(gradient_png);
    ground_line_texture = GRRLIB_LoadTexturePNG(ground_line_png);
    menu_ground = GRRLIB_LoadTexturePNG(g_01_png); //groundgroundgroundgourng
    

    //difficultty faces
    difficulty_faces[0] = GRRLIB_LoadTexturePNG(easy_png);
    difficulty_faces[1] = GRRLIB_LoadTexturePNG(normal_png);
    difficulty_faces[2] = GRRLIB_LoadTexturePNG(hard_png);
    difficulty_faces[3] = GRRLIB_LoadTexturePNG(harder_png);
    difficulty_faces[4] = GRRLIB_LoadTexturePNG(insane_png);
    difficulty_faces[5] = GRRLIB_LoadTexturePNG(demon_png);
    difficulty_faces[6] = GRRLIB_LoadTexturePNG(auto_png);

    create_button((screenWidth) / 2 - 200, 100, 400, 160,false,menu_arrow,start_level,false,false); //main play button
    create_button(5,200,56,120,true,menu_arrow,menu_go_left,false,false); //left arrow
    create_button((screenWidth) - 61,200,56,120,true,menu_arrow,menu_go_right,true,false); //right arrow

    draw_menu();
    fade_in();

    u64 prevTicks = gettime();

    bool exit_code = FALSE;

    while (1) {
        start_frame = gettime();
        float frameTime = ticks_to_secs_float(start_frame - prevTicks);
        dt = frameTime;

        update_input();
        if (!MP3Player_IsPlaying() && menuLoop) {
            MP3Player_PlayBuffer(menuLoop, size, NULL);
            MP3Player_Volume(255);
        }
        
        if (state.input.pressed1orX) {
            draw_menu();
            fade_out();
            level_id = 0;
            level_mode ^= 1;

            if (level_mode == 1) {
                // Read first gmd
                char *level_data = read_file(sd_level_paths[level_id].name, &outsize);
                if (level_data) {
                    snprintf(current_level_name, 255, "%s - by %s", get_level_name(level_data), get_author_name(level_data));
                    
                    check_custom_song(level_data);
                    
                    free(level_data);
                }
            }
            
            draw_menu();
            fade_in();
        }
        
        if (level_mode == 0) {
            if (main_levels()) {
                fade_out();
                break;
            }
        } else {
            if (sdcard_levels()) {
                fade_out();
                break;
            }
        }

        if (state.input.pressedHome) {
            exit_code = TRUE;
            break;
        }

        GRRLIB_Render();
    }

    GRRLIB_FreeTexture(menu_arrow);
    GRRLIB_FreeTexture(menu_corner_squares);
    GRRLIB_FreeTexture(menu_top_bar);
    GRRLIB_FreeTexture(gradient_texture);
    GRRLIB_FreeTexture(font_bold);
    for (int i = 0; i < FACES_COUNT; i++){
        GRRLIB_FreeTexture(difficulty_faces[i]);
    }
    if (menuLoop) free(menuLoop);
    
    if (level_info.song_offset > 0) {
        int textOffset = (get_text_length(big_font, 0.5, "Loading..."));
        
        draw_text(big_font, big_font_text, screenWidth - textOffset - 30 * 2, screenHeight - 30, 0.5, "Loading...");
    }

    GRRLIB_Render();
    GRRLIB_Render();;

    return exit_code;
}

void refresh_sdcard_levels() {
    memset(current_level_name, 0, 255);
    error_code = 0;
                
    if (sd_level_paths[level_id].is_dir) {
        snprintf(current_level_name, 255, "%s", sd_level_paths[level_id].name);
    }

    char *level_data = read_file(sd_level_paths[level_id].name, &outsize);
    if (level_data) {
        snprintf(current_level_name, 255, "%s - by %s", get_level_name(level_data), get_author_name(level_data));
        
        check_custom_song(level_data);

        free(level_data);
    }
}

int sdcard_levels() {
    GRRLIB_FillScreen(RGBA(0, 127, 0, 255));
            
    draw_text(big_font, big_font_text, 0, 400, 0.5, "Press 1 to switch to main levels.");
    
    if (sd_level_count > 0) {
        char text[269];
        if (sd_level_paths[level_id].is_dir) {
            snprintf(text, sizeof(text), "FOLDER - %s", current_level_name);

            draw_text(big_font, big_font_text, 0, 20, 0.5, text);
        } else {
            snprintf(text, sizeof(text), "%d - %s", level_id + 1, current_level_name);

            draw_text(big_font, big_font_text, 0, 20, 0.5, text);
            if (custom_song_id >= 0) print_custom_song(custom_song_id);
        }

        if (error_code) {
            draw_text(big_font, big_font_text, 0, 300, 0.5, "Failed to load the level with code %d\n", error_code);
        }

        if (state.input.pressedDir & INPUT_LEFT) {
            level_id--;
            if (level_id < 0) level_id = sd_level_count - 1; 
            refresh_sdcard_levels();
        }

        if (state.input.pressedDir & INPUT_RIGHT) {
            level_id++;
            if (level_id >= sd_level_count) level_id = 0;
            refresh_sdcard_levels();
        }

        if (state.input.pressedB && dir_level > 0) {
            go_back_directory(current_directory);
            load_folder(current_directory);
            level_id = 0;
            dir_level--;
            refresh_sdcard_levels();
        }

        if (state.input.pressedA) {
            if (sd_level_paths[level_id].is_dir) {
                dir_level++;
                load_folder(sd_level_paths[level_id].name);
                level_id = 0;
                refresh_sdcard_levels();
            } else {
                // Start level
                gameRoutine = ROUTINE_GAME;
                MP3Player_Stop();
                PlayOgg(playSound_01_ogg, playSound_01_ogg_size, 0, OGG_ONE_TIME);
                for (int i = 0; i < 90; i++) {
                    update_input();
                    VIDEO_WaitVSync();
                }
                
                char *level = read_file(sd_level_paths[level_id].name, &outsize);
                int code = load_level(level, TRUE);
                free(level);

                if (!code) {
                    return 1;
                }
                error_code = code;
                return 0;
            }
        }
    } else {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", launch_dir, USER_LEVELS_FOLDER);
        draw_text(big_font, big_font_text, 0, 20, 0.5, "Put levels in:");
        draw_text(big_font, big_font_text, 0, 45, 0.5, path);
        draw_text(big_font, big_font_text, 0, 70, 0.5, "For getting levels, use the mod");
        draw_text(big_font, big_font_text, 0, 95, 0.5, "\"GDShare\" for exporting.");
    }
    return 0;
}

int main_levels() {
    if (level_id < NUM_LVL_COLORS) {
        GRRLIB_FillScreen(default_lvl_colors[level_id]);
    } else {
        GRRLIB_FillScreen(RGBA(0, 127, 255, 255));
    }
    
    GRRLIB_DrawImg(0,0,gradient_texture,0,screenWidth/256.f,screenHeight/256.f,RGBA(255,255,255,127));//bg gradient
    for (int i = 0; i < screenWidth; i += 192){
        if (level_id < NUM_LVL_COLORS) {
            GRRLIB_DrawImg(i,screenHeight-64,menu_ground,0,1.5,1.5,default_lvl_colors[level_id]);//add ground
        } else {
            GRRLIB_DrawImg(i,screenHeight-64,menu_ground,0,1.5,1.5,RGBA(0, 127, 255, 255));//add ground 2: electric boogaloo
        }
    }
    GRRLIB_DrawImg(screenWidth/2 - 444 * (screen_factor_x * 0.5),screenHeight-64,ground_line_texture,0,screen_factor_x * 0.5,0.75,RGBA(255,255,255,255));//ground line
    GRRLIB_DrawImg(screenWidth/2 - 306 * 0.75,-1,menu_top_bar,0,0.75,0.75,RGBA(255,255,255,255));//the bar at the top
    GRRLIB_DrawImg(0, screenHeight - 143 * 0.75,menu_corner_squares,0,0.75,0.75,RGBA(255,255,255,255));//corner thing left
    GRRLIB_DrawImg(screenWidth, screenHeight - 143 * 0.75,menu_corner_squares,0,-0.75,0.75,RGBA(255,255,255,255));//corner thing right
          
    draw_text(big_font, big_font_text, 0, 400, 0.5, "Press 1 to switch to custom levels.");
    
    //level name display
    custom_rounded_rectangle((screenWidth) / 2 - 250, 100, 500, 160, 10, RGBA(0, 0, 0, 127));
    int textOffset = (get_text_length(big_font, 0.5, levels[level_id].level_name) - 70) / 2;
    //GRRLIB_Printf(screenWidth/2 - textOffset, 160, font_bold, RGBA(0,0,0,255), 0.75, levels[level_id].level_name);
    //GRRLIB_Printf(screenWidth/2 - textOffset, 160, font, RGBA(255,255,255,255), 0.75, levels[level_id].level_name);
    draw_text(big_font, big_font_text, screenWidth/2 - textOffset, 160, 0.5, levels[level_id].level_name);
    
    GRRLIB_DrawImg(screenWidth/2 - textOffset - 70,150,difficulty_faces[default_level_difficulty[level_id]],0,0.75,0.75,RGBA(255,255,255,255));

    //the circles at the bottom of the screen
    int dotsStartX = (screenWidth / 2) - ((LEVEL_NUM * 16) / 2);
    int selectedColor = RGBA(255, 255, 255, 255);
    int otherColor = RGBA(127, 127, 127, 255);
    for (int i = 0; i < LEVEL_NUM; i++) {
        custom_circle(dotsStartX + i * 16, screenHeight - 32, 4, level_id == i?selectedColor:otherColor);
    }

    //buttons
    for(int i = 0; i < button_count; i++) {
        if (button_list[i].show){
            if (button_list[i].flip_x){
                GRRLIB_DrawImg(button_list[i].x + button_list[i].width, button_list[i].y, button_list[i].texture,0,-1,1, RGBA(255, 255, 255, 255));
            }else{
                GRRLIB_DrawImg(button_list[i].x, button_list[i].y, button_list[i].texture,0,1,1, RGBA(255, 255, 255, 255));
            }
            
        }
    }

    GRRLIB_DrawImg(ir_x,ir_y, cursor,0,1,1,RGBA(255,255,255,255)); // draw cursor

    if (state.input.pressedDir & INPUT_LEFT) {
        menu_go_left();
    }

    if (state.input.pressedDir & INPUT_RIGHT) {
        menu_go_right();
    }
    
    if (state.input.pressedA) {
        // ale is no longer going to kill me
        for (int i = 0; i < button_count; i++){
            if (GRRLIB_PtInRect(button_list[i].x,button_list[i].y,button_list[i].width,button_list[i].height,ir_x,ir_y)){
            button_list[i].func();
            }
        }
    }
    if (state.input.pressedPlusOrL){
        start_level();
        return 1;
    }
    
    if (exit_menu) return 1;
    return 0;
}

