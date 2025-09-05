#pragma once
#include <grrlib.h>
#include "structs.h"
#include "level_loading.h"
#include "font_stuff.h"

#define SAME_SIGN(v1,v2) (v1>=0)==(v2>=0)
#define DIFFERENT_SIGN(v1,v2) (v1>=0)!=(v2>=0)

#define FADE_DURATION 0.25f
#define FADE_SPEED (255 / FADE_DURATION)


extern Mtx                 GXmodelView2D;

Vec2D rotate(Vec2D point, float angle, Vec2D origin);
float maxf(float a, float b);
float positive_fmod(float n, float divisor);
Color color_lerp(Color color1, Color color2, float fraction);
float approachf(float current, float target, float speed, float smoothing);
void rotate_point_around_center_gfx(
    float x, float y,               // anchor position on screen
    float offset_x, float offset_y, // object position (pre-rotation)
    float center_x, float center_y, // pivot center in the object
    float width, float height,      // size of the object
    float rotation_deg,             // rotation angle in degrees
    float *output_x, float *output_y // output screen position
);

void rotate_point_around_center(float cx, float cy, float angle, float x, float y, float *out_x, float *out_y);

float adjust_angle(float angle, int flipX, int flipY);
float adjust_angle_x(float angle, int flipX);
float adjust_angle_y(float angle, int flipY);
float randomf();
float random_float(float min, float max);
int random_int(int min, int max);
float map_range(float val, float min1, float max1, float min2, float max2);

float square_distance(float xa, float ya, float xb, float yb);

float clampf(float d, float min, float max);
float slerp(float a, float b, float ratio);
float lerp(float from, float to, float alpha);
float iLerp(float a, float b, float ratio, float dt);
float iSlerp(float a, float b, float ratio, float dt);
float ease_out(float current, float target, float smoothing);

void  custom_drawImg (const f32 xpos, const f32 ypos, const GRRLIB_texImg *tex, const f32 degrees, const f32 scaleX, const f32 scaleY, const u32 color);
void  custom_drawPart (const f32 xpos, const f32 ypos, const f32 partx, const f32 party, const f32 partw, const f32 parth, const GRRLIB_texImg *tex, const f32 degrees, const f32 scaleX, const f32 scaleY, const u32 color);
void  custom_circle (const f32 x, const f32 y, const f32 radius,
                     const u32 color);
void  custom_circunference (const f32 x, const f32 y, const f32 radius,
                     const u32 color, const f32 lineWidth);
void  custom_rectangle (const f32 x,      const f32 y,
    const f32 width,  const f32 height,
    const u32 color, const bool filled);
void custom_rounded_rectangle(float x, float y,
                                     float width, float height,
                                     float radius,
                                     u32 color);
void custom_line (const f32 x1, const f32 y1,
    const f32 x2, const f32 y2, const u32 color);
void draw_thick_line(const float x1, const float y1, const float x2, const float y2, const float thickness, const u32 color);
void draw_hitbox_line_inward(Vec2D rect[4], 
                             const float x1, const float y1,
                             const float x2, const float y2,
                             const float thickness,
                             const float cx, const float cy,
                             const u32 color);
void draw_polygon_inward_mitered(Vec2D *poly, int n, float thickness, u32 color);

void set_texture(const GRRLIB_texImg *tex);
float normalize_angle(float angle);
float ip1_ceilf(float x);

void fade_in();
void fade_out();
void fade_in_level();
void wait_initial_time();

float get_text_length(struct charset font, const float zoom, const char *text, ...);
void draw_text(struct charset font, GRRLIB_texImg *tex, const float x, const float y, const float zoom, const char *text, ...);

Color HSV_combine(Color color, HSV hsv);